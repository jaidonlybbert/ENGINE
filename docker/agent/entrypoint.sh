#!/usr/bin/env bash
# Runs as PID 1 inside each agent container (see issue #64). Clones the target repo into
# this container's own named volume on first start - never a bind mount of the host
# checkout, so nothing an agent does here can touch files outside the container - then
# hands off to whatever CMD the compose service was given (default: an interactive shell).
set -euo pipefail

REPO_URL="${REPO_URL:-https://github.com/jaidonlybbert/ENGINE.git}"
REPO_BRANCH="${REPO_BRANCH:-main}"

if [ ! -d /workspace/.git ]; then
    echo "==> Cloning ${REPO_URL} (branch ${REPO_BRANCH}) into /workspace"
    git clone --branch "$REPO_BRANCH" --recurse-submodules "$REPO_URL" /workspace
fi

git config --global user.name "${GIT_AUTHOR_NAME:-engine-agent}"
git config --global user.email "${GIT_AUTHOR_EMAIL:-engine-agent@users.noreply.github.com}"
git config --global --add safe.directory /workspace

# Lets `git push`/`gh pr create` authenticate with GITHUB_TOKEN without ever touching an
# SSH key - see docker/README.md on scoping this token. No-op (silently) if unset.
if [ -n "${GITHUB_TOKEN:-}" ]; then
    gh auth setup-git >/dev/null 2>&1 || true
fi

# Idempotent - safe to re-run every start.
conan profile detect --force >/dev/null 2>&1 || true

exec "$@"
