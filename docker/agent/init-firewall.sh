#!/bin/bash
# Egress firewall for the agent containers (see issue #67). Adapted from the reference
# firewall in Anthropic's Claude Code devcontainer: default-deny outbound traffic, then allow
# only an explicit list of hosts. Run as root via sudo by entrypoint.sh at container start
# (the `agent` user can run this one script and nothing else as root - see the sudoers line
# in the Dockerfile), and fails closed: if any step fails, the container doesn't start.
#
# Differences from the reference, all tightenings or fixes for this setup:
#  - no blanket outbound SSH (this image has no SSH keys, git goes over HTTPS)
#  - DNS only to the resolvers in /etc/resolv.conf, not to any host
#  - the container's own Docker subnet is allowed (agents must reach each other), but the
#    subnet's gateway - the Docker host - is not, and neither is the rest of the host's LAN
#  - IPv6 is dropped (the reference only configures IPv4)
#  - `ipset add -exist`, since two allowed domains can resolve to the same IP
#  - the allowlist is for what agents here need (Claude, GitHub, npm/PyPI/Conan), extended
#    via FIREWALL_EXTRA_DOMAINS (space-separated) instead of editing this file
set -euo pipefail
IFS=$'\n\t'

# Hosts agents need. Some Conan recipes download sources from hosts not listed here - add
# them through FIREWALL_EXTRA_DOMAINS rather than turning the firewall off.
ALLOWED_DOMAINS=(
    api.anthropic.com
    claude.ai
    platform.claude.com
    downloads.claude.ai
    registry.npmjs.org
    pypi.org
    files.pythonhosted.org
    center2.conan.io
)
if [ -n "${FIREWALL_EXTRA_DOMAINS:-}" ]; then
    IFS=' ' read -r -a EXTRA_DOMAINS <<< "$FIREWALL_EXTRA_DOMAINS"
    ALLOWED_DOMAINS+=("${EXTRA_DOMAINS[@]}")
fi

# 1. Extract Docker's embedded-DNS NAT rules BEFORE any flushing
DOCKER_DNS_RULES=$(iptables-save -t nat | grep "127\.0\.0\.11" || true)

# Flush existing rules and delete existing ipsets
iptables -F
iptables -X
iptables -t nat -F
iptables -t nat -X
iptables -t mangle -F
iptables -t mangle -X
ipset destroy allowed-domains 2>/dev/null || true

# 2. Selectively restore ONLY internal Docker DNS resolution
if [ -n "$DOCKER_DNS_RULES" ]; then
    echo "Restoring Docker DNS rules..."
    iptables -t nat -N DOCKER_OUTPUT 2>/dev/null || true
    iptables -t nat -N DOCKER_POSTROUTING 2>/dev/null || true
    echo "$DOCKER_DNS_RULES" | xargs -L 1 iptables -t nat
else
    echo "No Docker DNS rules to restore"
fi

# Localhost
iptables -A INPUT -i lo -j ACCEPT
iptables -A OUTPUT -o lo -j ACCEPT

# DNS, only to the configured resolvers (with Docker's embedded resolver at 127.0.0.11
# this rides over the loopback rules above). Replies come back via the ESTABLISHED rule.
while read -r ns; do
    iptables -A OUTPUT -p udp -d "$ns" --dport 53 -j ACCEPT
    iptables -A OUTPUT -p tcp -d "$ns" --dport 53 -j ACCEPT
done < <(awk '$1 == "nameserver" && $2 ~ /^[0-9.]+$/ {print $2}' /etc/resolv.conf)

# Create ipset with CIDR support
ipset create allowed-domains hash:net

# Fetch GitHub meta information and aggregate + add their IP ranges
echo "Fetching GitHub IP ranges..."
gh_ranges=$(curl -fsS --max-time 20 https://api.github.com/meta) || {
    echo "ERROR: Failed to fetch GitHub IP ranges"
    exit 1
}

if ! echo "$gh_ranges" | jq -e '.web and .api and .git' >/dev/null; then
    echo "ERROR: GitHub API response missing required fields"
    exit 1
fi

echo "Processing GitHub IPs..."
while read -r cidr; do
    if [[ ! "$cidr" =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}/[0-9]{1,2}$ ]]; then
        echo "ERROR: Invalid CIDR range from GitHub meta: $cidr"
        exit 1
    fi
    echo "Adding GitHub range $cidr"
    ipset add -exist allowed-domains "$cidr"
done < <(echo "$gh_ranges" | jq -r '(.web + .api + .git)[]' | aggregate -q)

# Resolve and add other allowed domains
for domain in "${ALLOWED_DOMAINS[@]}"; do
    echo "Resolving $domain..."
    ips=$(dig +noall +answer A "$domain" | awk '$4 == "A" {print $5}')
    if [ -z "$ips" ]; then
        echo "ERROR: Failed to resolve $domain"
        exit 1
    fi

    while read -r ip; do
        if [[ ! "$ip" =~ ^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
            echo "ERROR: Invalid IP from DNS for $domain: $ip"
            exit 1
        fi
        echo "Adding $ip for $domain"
        ipset add -exist allowed-domains "$ip"
    done < <(echo "$ips")
done

# The container's own subnet (other agents) is allowed; the gateway on it (the Docker host)
# and everything beyond the subnet is not.
GATEWAY=$(ip -4 route show default | awk '{print $3; exit}')
DEV=$(ip -4 route show default | awk '{print $5; exit}')
SUBNET=$(ip -4 route show dev "$DEV" scope link proto kernel | awk '{print $1; exit}')
if [ -z "$GATEWAY" ] || [ -z "$SUBNET" ]; then
    echo "ERROR: Failed to detect gateway/subnet (gateway='$GATEWAY' subnet='$SUBNET')"
    exit 1
fi
echo "Gateway $GATEWAY blocked; container subnet $SUBNET allowed"

iptables -A OUTPUT -d "$GATEWAY" -j REJECT --reject-with icmp-admin-prohibited
iptables -A INPUT -s "$GATEWAY" -j DROP
iptables -A INPUT -s "$SUBNET" -j ACCEPT
iptables -A OUTPUT -d "$SUBNET" -j ACCEPT

# Set default policies to DROP first
iptables -P INPUT DROP
iptables -P FORWARD DROP
iptables -P OUTPUT DROP

# First allow established connections for already approved traffic
iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
iptables -A OUTPUT -m state --state ESTABLISHED,RELATED -j ACCEPT

# Then allow only specific outbound traffic to allowed domains
iptables -A OUTPUT -m set --match-set allowed-domains dst -j ACCEPT

# Explicitly REJECT all other outbound traffic for immediate feedback
iptables -A OUTPUT -j REJECT --reject-with icmp-admin-prohibited

# IPv6: allow nothing but loopback. (Docker's default networks are IPv4-only, but a
# network with IPv6 enabled would otherwise bypass everything above.)
if command -v ip6tables >/dev/null && ip6tables -L >/dev/null 2>&1; then
    ip6tables -F
    ip6tables -A INPUT -i lo -j ACCEPT
    ip6tables -A OUTPUT -o lo -j ACCEPT
    ip6tables -P INPUT DROP
    ip6tables -P FORWARD DROP
    ip6tables -P OUTPUT DROP
else
    echo "WARNING: ip6tables unavailable - IPv6 traffic is NOT filtered"
fi

echo "Firewall configuration complete"
echo "Verifying firewall rules..."
if curl --connect-timeout 5 https://example.com >/dev/null 2>&1; then
    echo "ERROR: Firewall verification failed - was able to reach https://example.com"
    exit 1
else
    echo "Firewall verification passed - unable to reach https://example.com as expected"
fi

# Verify GitHub API access
if ! curl --connect-timeout 5 https://api.github.com/zen >/dev/null 2>&1; then
    echo "ERROR: Firewall verification failed - unable to reach https://api.github.com"
    exit 1
else
    echo "Firewall verification passed - able to reach https://api.github.com as expected"
fi
