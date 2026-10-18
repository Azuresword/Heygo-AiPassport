#!/usr/bin/env bash
# Build the summary book into ../summary for GitHub Pages.
set -euo pipefail
cd "$(dirname "$0")"
export PATH="/opt/homebrew/bin:$PATH"
npx --yes honkit@6.2.2 build . ../summary
