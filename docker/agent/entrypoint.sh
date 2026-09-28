#!/usr/bin/env bash
# Runs as PID 1 inside each agent container (see issue #64). Clones the target repo into
# this container's own named volume on first start - never a bind mount of the host
# checkout, so nothing an agent does here can touch files outside the container - then
# hands off to whatever CMD the compose service was given (default: an interactive shell).
set -euo pipefail

REPO_URL="${REPO_URL:-https://github.com/jaidonlybbert/ENGINE.git}"
REPO_BRANCH="${REPO_BRANCH:-main}"

AUTHOR_NAME="${GIT_AUTHOR_NAME:-engine-agent}"
AUTHOR_EMAIL="${GIT_AUTHOR_EMAIL:-engine-agent@users.noreply.github.com}"
git config --global user.name "$AUTHOR_NAME"
git config --global user.email "$AUTHOR_EMAIL"
git config --global --add safe.directory /workspace

# This container has no SSH keys on purpose (see docker/README.md), but .gitmodules uses
# git@github.com: URLs - so rewrite them to HTTPS, for the initial clone below and for
# anything the agent runs later. Must happen before the clone.
git config --global url."https://github.com/".insteadOf "git@github.com:"
git config --global --add url."https://github.com/".insteadOf "ssh://git@github.com/"

# Lets `git push`/`gh pr create` authenticate with GITHUB_TOKEN without ever touching an
# SSH key - see docker/README.md on scoping this token. No-op (silently) if unset.
if [ -n "${GITHUB_TOKEN:-}" ]; then
    gh auth setup-git >/dev/null 2>&1 || true
fi

if [ ! -d /workspace/.git ]; then
    echo "==> Cloning ${REPO_URL} (branch ${REPO_BRANCH}) into /workspace"
    git clone --branch "$REPO_BRANCH" "$REPO_URL" /workspace
fi

# Its own step (not --recurse-submodules on the clone) so a submodule failure can't leave a
# half-cloned workspace that the check above would then treat as done - and re-run every
# start, since it's a no-op once they're checked out.
git -C /workspace submodule update --init --recursive

# Idempotent - safe to re-run every start.
conan profile detect --force >/dev/null 2>&1 || true

exec "$@"
