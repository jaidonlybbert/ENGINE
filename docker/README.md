# Isolated agent environment

A Docker Compose setup for running coding agents (e.g. [Claude Code](https://claude.com/claude-code))
against this project unattended, without giving them direct access to your host machine
(see [issue #64](https://github.com/jaidonlybbert/ENGINE/issues/64)). Two agent containers
(`agent1`, `agent2`) are defined as a starting example of agents that can reach each other;
copy the pattern in `docker-compose.yml` to add more.

This is a sandbox for *running* agents, not a build environment for *you* - if you just
want a consistent toolchain for your own interactive development, use
[`.devcontainer/`](../.devcontainer) instead, which bind-mounts your actual working
directory (intentionally - that one is meant to be convenient, not isolated).

## Quickstart

```bash
cd docker
cp .env.example .env
# edit .env: at minimum, set ANTHROPIC_API_KEY (see "Configuring API keys" below)

docker compose build
docker compose up -d
docker compose exec agent1 claude
```

The first time a container starts, its entrypoint clones `REPO_URL`/`REPO_BRANCH` (from
`.env`) into its own volume - not your host checkout, see below - so the first `up` will
take a minute. `claude` (or any other command) runs as the unprivileged `agent` user with
`/workspace` as its working directory.

To stop everything: `docker compose down` (add `-v` to also delete the cloned
workspace/shared volumes and start fresh next time).

## What's isolated, and how

- **No bind mount of your host filesystem.** Each agent clones the repo into its own named
  Docker volume (`agent1-workspace`, `agent2-workspace`) at container start. Nothing an
  agent does to `/workspace` touches your actual working directory - if you want to look at
  or pull out what an agent produced, use `git push`/a PR (see "Configuring API keys"),
  `docker compose cp agent1:/workspace/some/file .`, or `docker run --rm -v
  engine-agents_agent1-workspace:/w -v "$PWD":/out busybox cp -r /w/. /out/`.
- **Non-root, capability-dropped.** Each container runs as an unprivileged `agent` user
  (not root), with every Linux capability dropped (`cap_drop: [ALL]`) and
  `no-new-privileges` set, so even a process that found a way to misbehave inside the
  container has very little to work with.
- **No Docker socket.** Nothing here mounts `/var/run/docker.sock` into a container - doing
  so would hand out root on the host (a container with the socket can start new containers
  with arbitrary mounts). If a future workflow needs agents to build/run containers
  themselves, that needs its own careful design, not a quick socket mount.
- **No SSH keys.** Git push/PR authentication goes through the GitHub CLI's HTTPS+token
  credential helper (`GITHUB_TOKEN`, see below) - never through a mounted or generated SSH
  private key, so there's no host SSH credential an agent could ever read or exfiltrate.
- **Isolated network.** All agents share a private `agents` Docker network; nothing is
  published to the host (no `ports:`), and nothing runs with `network_mode: host`. Agents
  can reach each other by hostname (`agent1`, `agent2`) but can't reach anything else
  running on your host.
- **Resource limits.** Each container is capped (`mem_limit`, `cpus`, `pids_limit` in
  `docker-compose.yml`) so one agent (or a build it kicks off) can't starve your machine or
  the other agent.

## What this does *not* protect against

Being transparent about the limits of this setup matters more than the setup itself:

- **Network egress is not filtered.** Agents can reach the general internet (they need to,
  for the Anthropic API and package registries like PyPI/npm/Conan Center). A compromised
  or badly-instructed agent could still make outbound requests to somewhere you didn't
  intend, including sending it data from inside its own container. If you need to lock this
  down further, put an egress-filtering proxy (an allowlist of just the hosts each tool
  needs) in front of the `agents` network - out of scope for this PR.
- **The agent can still open pull requests, or push, if you give it `GITHUB_TOKEN`.**
  That's the point (an agent that can't ever contribute anything isn't very useful), but it
  means *you* are the safety net - review what it changed before merging, the same as
  you'd review any other contributor's PR. Don't hand out a token with more scope than
  "this one repo" (see below), and don't set up auto-merge for agent-authored PRs.
  Consider pointing `REPO_URL` at a fork instead of this repo directly if you want an
  extra step between "agent opens a PR" and "it's visible upstream."
- **A leaked `ANTHROPIC_API_KEY` is still a leaked API key.** Isolation here is about the
  container's access to *your machine*, not about the value of the credential you hand it.
  See "Configuring API keys" for how to bound the damage a leaked key (or a runaway agent
  burning through your quota) can do.

## Configuring API keys

`docker/.env` (copied from `.env.example`, gitignored - **never commit it**) is the only
place secrets go. It's loaded both by Docker Compose itself (for `${VAR}` substitution in
`docker-compose.yml`) and injected into every agent container via `env_file:`.

- **`ANTHROPIC_API_KEY`** - required. Create a key specifically for this purpose in the
  [Anthropic Console](https://console.anthropic.com/settings/keys), separate from any key
  you use elsewhere, and set a spend limit on it under
  [Settings -> Limits](https://console.anthropic.com/settings/limits). That limit is your
  actual backstop against a runaway or compromised agent - the container isolation above
  stops an agent from reaching your host, not from calling the API a lot.
- **`GITHUB_TOKEN`** - optional, only needed if agents should push branches or open PRs.
  Use a [fine-grained personal access token](https://github.com/settings/personal-access-tokens/new),
  scoped to **only this repository** (not "all repositories"), with only the permissions
  actually needed - typically `Contents: Read and write` and `Pull requests: Read and
  write`, nothing else. Leave it empty if you only want agents to read the repo.
- Rotate both periodically, and revoke immediately if a container image or log ever looks
  like it might have leaked one.

## How agents talk to each other

- **By hostname.** Both containers sit on the same `agents` Docker network with fixed
  hostnames (`agent1`, `agent2`), so anything one agent runs that listens on a port (an
  HTTP server, etc.) is reachable from the other at `http://agent2:<port>/...` - no
  service discovery needed.
- **Through `/shared`.** A named volume mounted read-write at `/shared` in every agent
  container, for exchanging files, notes, or handoff state without needing a network
  service at all.

Neither of those *is* a communication protocol - what agents actually do with that
reachability (an HTTP API, polling a shared file, something else) is up to whatever agent
tooling you run inside them.

## Adding more agents

Copy an `agentN:` block in `docker-compose.yml`, give it its own `container_name`,
`hostname`, and workspace volume (add the volume under top-level `volumes:` too), and it'll
pick up everything else - the build, resource limits, capability drops, and network - from
the `x-agent-common` anchor at the top of the file.
