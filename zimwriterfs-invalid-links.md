# zimwriterfs and official Wikipedia ZIMs (invalid-looking valid links)

## Problem

Rewriting official Wikipedia ZIMs with `zimwriterfs` can error on URLs that are valid in context
(encoding, relative forms, or fragment-only references) but fail naive validation.

## Guidance

1. Capture the exact failing URL from the tool log.
2. Check whether the link resolves inside the ZIM (same article / redirect) vs external.
3. Prefer preserving Wikipedia-relative forms rather than forcing absolute `https://` rewrite when the entry exists.
4. If validation is too strict for known-good Wikimedia patterns, track a allowlist or normalize step before fail-fast.

## Issue context

See the discussion in the linked GitHub issue for sample paths and error text.

## Related

- `zimwriterfs` entry rewriting options in `--help`
- libzim link resolution behavior for relative paths
