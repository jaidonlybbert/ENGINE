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
# .env's defaults work as-is - `claude` will just ask you to log in interactively the
# first time you run it in a container. See "Authenticating Claude Code" below for that,
# and for a faster option if you're starting up several agents at once.

docker compose build
docker compose up -d
docker compose exec -it agent1 claude
```

The first time a container starts, its entrypoint clones `REPO_URL`/`REPO_BRANCH` (from
`.env`) into its own volume - not your host checkout, see below - so the first `up` will
take a minute. `claude` (or any other command) runs as the unprivileged `agent` user with
`/workspace` as its working directory.

To stop everything: `docker compose down` (add `-v` to also delete the cloned
workspace/home/shared volumes, including any saved login, and start fresh next time).

## What's isolated, and how

- **No bind mount of your host filesystem.** Each agent clones the repo into its own named
  Docker volume (`agent1-workspace`, `agent2-workspace`) at container start, and its `$HOME`
  (`agent1-home`, `agent2-home` - where logging into `claude` saves its session, see
  "Authenticating Claude Code") is a separate named volume too. Nothing an agent does
  touches your actual working directory or your own Claude Code login - if you want to look
  at or pull out what an agent produced, use `git push`/a PR (see "Authenticating Claude
  Code"), `docker compose cp agent1:/workspace/some/file .`, or `docker run --rm -v
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
- **A leaked Claude Code login or `ANTHROPIC_API_KEY` is still a leaked credential.**
  Isolation here is about the container's access to *your machine*, not about the value of
  whatever you authenticate it with. See "Authenticating Claude Code" for how to bound the
  damage either one leaking (or a runaway agent burning through usage) can do.

## Authenticating Claude Code

Three ways to authenticate, in the order Claude Code itself prefers them if more than one
is present (so top one wins) - pick one per agent, or per this whole setup:

### Option A: `claude setup-token` (recommended - simplest for running several agents)

Run this once, anywhere you already have Claude Code installed and logged in with your
Pro/Max/Team/Enterprise subscription - your own host machine works fine, it just needs a
browser and has nothing to do with Docker:

```bash
claude setup-token
```

It opens the same browser login `claude`/`/login` uses and prints a long-lived (one year)
token once you approve. Put it in `docker/.env`:

```
CLAUDE_CODE_OAUTH_TOKEN=<the token>
```

Every agent container picks it up via `env_file:` with no per-container login step - `claude`
just works the moment the container starts. All agents authenticate as your subscription
this way, sharing its usage/rate limits (see [Claude Code's usage
docs](https://docs.claude.com/en/docs/claude-code/costs) for what your plan allows). If you
want agents on independent, non-competing usage instead, get a token per subscription/
account and give each agent its own in a per-service `environment:` override instead of the
shared `env_file:`.

### Option B: Interactive per-container login

```bash
docker compose up -d
docker compose exec -it agent1 claude
```

`claude` opens a browser login. If a browser can't open automatically (common for
containers, SSH, and WSL2 - Claude Code detects this itself), press `c` to copy the login
URL, open it in a browser on *any* device, and sign in. If that browser shows you a code
instead of redirecting back, paste it at the `Paste code here if prompted` prompt in the
container's terminal - this is expected, it just means the browser couldn't reach the
container's local callback server, which is exactly the case here.

Once it says `Login successful`, the session is saved to `~/.claude/.credentials.json`
*inside that container*, persisted by its `agent1-home` volume - you won't need to log in
again unless you `docker compose down -v` (which deletes it) or run `/logout`. Repeat for
`agent2` (and any agent you add); each is a separate login, so you can put different
subscriptions/accounts on different agents this way without the per-service `environment:`
override Option A needs for the same result.

### Option C: `ANTHROPIC_API_KEY`

Set it in `docker/.env` instead of A or B, if you'd rather bill this to a metered API key
than a subscription seat. **Careful: this one wins if it's set alongside either option
above** - Claude Code prefers `ANTHROPIC_API_KEY` over both a `CLAUDE_CODE_OAUTH_TOKEN` and
an interactive subscription login, asking you to approve it once. Leave it blank unless
that's what you want; `/status` inside `claude` shows which credential is actually active.
If you do set it, create a key specifically for this purpose in the [Anthropic
Console](https://console.anthropic.com/settings/keys), separate from any key you use
elsewhere, and set a spend limit on it under [Settings ->
Limits](https://console.anthropic.com/settings/limits) - that limit is your actual backstop
against a runaway or compromised agent, the container isolation above stops an agent from
reaching your host, not from calling the API a lot.

`docker/.env` (copied from `.env.example`, gitignored - **never commit it**) is loaded both
by Docker Compose itself for `${VAR}` substitution and injected into every container via
`env_file:`, which is how all three options above actually reach `claude` inside a
container.

### `GITHUB_TOKEN` (either option, optional)

Only needed if agents should push branches or open PRs - set it in `docker/.env`. Use a
[fine-grained personal access
token](https://github.com/settings/personal-access-tokens/new), scoped to **only this
repository** (not "all repositories"), with only the permissions actually needed -
typically `Contents: Read and write` and `Pull requests: Read and write`, nothing else.
Leave it empty if you only want agents to read the repo. Rotate it periodically, and revoke
immediately if a container image or log ever looks like it might have leaked it.

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
`hostname`, and workspace + home volumes (add both under top-level `volumes:` too, and
mount them the same way `agent1`/`agent2` do), and it'll pick up everything else - the
build, resource limits, capability drops, and network - from the `x-agent-common` anchor at
the top of the file. It'll need its own authentication the same as any other agent - see
"Authenticating Claude Code".
