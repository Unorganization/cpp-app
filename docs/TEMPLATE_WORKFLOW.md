# Template Workflow

This document describes how to use `cpp-app` as a template for creating new
tool repositories. It covers the one-time setup for a new tool, the daily
development workflow, and how to pull updates from the template into an
existing tool.

---

## Concepts

**`cpp-app` (the template):** The source of shared infrastructure — build
system, CI pipeline, platform flags, documentation conventions, and library
demos. Changes to the template (new platforms, build system improvements,
convention updates) are made here and propagated outward.

**`tool-*` repos (the tools):** Independent GitHub repositories derived from
the template. Each has its own name, its own releases, and its own history.
They share the template's infrastructure but diverge in `src/` and
`CMakeLists.txt`.

**The `template` remote:** A Git remote added to each tool repo that points
back to `cpp-app`. This is how template updates are pulled in. You never push
to it.

---

## Part 1: Creating a New Tool Repository

Do this once per new tool.

### Step 1 — Create the empty repo on GitHub

Create a new repository under your organisation named `tool-<name>` (e.g.
`tool-myapp1`). Do not initialise it with a README or any files — leave it
completely empty.

### Step 2 — Clone the template locally

```bash
git clone https://github.com/your-org/cpp-app tool-myapp1
cd tool-myapp1
```

You now have a local copy with the full template history. The `origin` remote
currently points to `cpp-app` — you will fix that next.

### Step 3 — Re-point origin to the new repo

```bash
git remote set-url origin https://github.com/your-org/tool-myapp1
```

Verify:

```bash
git remote -v
# origin   https://github.com/your-org/tool-myapp1 (fetch)
# origin   https://github.com/your-org/tool-myapp1 (push)
```

### Step 4 — Add the template as a named remote

```bash
git remote add template https://github.com/your-org/cpp-app
```

Verify:

```bash
git remote -v
# origin    https://github.com/your-org/tool-myapp1 (fetch)
# origin    https://github.com/your-org/tool-myapp1 (push)
# template  https://github.com/your-org/cpp-app (fetch)
# template  https://github.com/your-org/cpp-app (push)
```

You will only ever fetch from `template`, never push.

### Step 5 — Push to the new repo

```bash
git push -u origin main
```

### Step 6 — Customise for your tool

Make these changes before writing any tool-specific code:

| File | What to change |
|---|---|
| `CMakeLists.txt` | `project(cpp_app ...)` → `project(tool_myapp1 ...)` |
| `vcpkg.json` | `"name": "cpp-app"` → `"name": "tool-myapp1"` |
| `README.md` | Replace with tool-specific description, setup, and usage |
| `docs/ARCHITECTURE.md` | Replace or trim to reflect the tool's actual design |
| `src/main.cpp` | Replace demo content with tool entry point |

Commit these changes:

```bash
git add -A
git commit -m "Initialise tool-myapp1 from cpp-app template"
```

### Step 7 — Configure `.gitattributes` for painless merges

Certain files will always diverge from the template and should never be
overwritten by a template merge. Tell Git to always keep your version of them:

```bash
cat >> .gitattributes << 'EOF'

# Always keep the tool's version of these files during template merges.
src/main.cpp        merge=ours
README.md           merge=ours
EOF
```

```bash
git add .gitattributes
git commit -m "Configure merge strategy for tool-specific files"
```

> **Note:** `merge=ours` means Git automatically keeps your version during
> `git merge`. You still see the conflict markers if you use `git rebase`, so
> stick to `git merge` when pulling from the template (see Part 3).

---

## Part 2: Daily Development Workflow

Nothing unusual here — this is standard Git.

```bash
# Work on your tool
git add -A
git commit -m "Add feature X"

# Push to your tool repo
git push
```

Releases are triggered by pushing a version tag:

```bash
git tag v1.0.0
git push origin v1.0.0
```

The CI pipeline runs on the tagged commit and publishes a release to
`https://github.com/your-org/tool-myapp1/releases`.

> Tags are scoped to each repo. `v1.0.0` in `tool-myapp1` and `v1.0.0` in
> `tool-myapp2` are completely independent — no namespacing needed.

---

## Part 3: Pulling Template Updates

Do this whenever `cpp-app` has changes you want — a new platform, a build
system fix, a CI improvement, a convention update.

### Step 1 — Fetch the latest template

```bash
git fetch template
```

This downloads the template's history but changes nothing in your working tree.

### Step 2 — Review what changed

```bash
git log HEAD..template/main --oneline
```

This shows every template commit you don't have yet. Read these before merging
so you know what is coming in and whether anything needs manual attention.

### Step 3 — Merge

```bash
git merge template/main
```

Files covered by `merge=ours` in `.gitattributes` are kept automatically.
Everything else merges normally.

### Step 4 — Resolve any remaining conflicts

```bash
git status   # shows files with conflicts
```

Expected conflict sites and how to handle them:

| File | Typical conflict | What to do |
|---|---|---|
| `src/main.cpp` | Entire file | Handled by `merge=ours` automatically |
| `README.md` | Entire file | Handled by `merge=ours` automatically |
| `CMakeLists.txt` | `project()` name, `vcpkg.json` dependencies block | Keep your project name; review dependency changes carefully and integrate selectively |
| `AGENTS.md` | New rules added to both sides | Read both versions; combine manually |
| `docs/ARCHITECTURE.md` | Structural changes | Review and integrate manually |
| `.github/workflows/build.yml` | Usually none | If conflicts arise, keep the template version unless you have tool-specific CI changes |

After resolving:

```bash
git add -A
git commit   # Git pre-fills the merge commit message — accept it
```

### Step 5 — Verify the build

After a template merge, always do a local build before pushing:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

---

## Gotchas

**Don't push to `template`.**
The remote is fetch-only by convention. There is nothing preventing a push
technically, but it would modify the shared template. Make template changes
by working directly in `cpp-app`.

**Always merge, never rebase, from the template.**
`git rebase template/main` rewrites your tool's commit history to look as if
it was always built on top of the template. This is confusing, breaks the
`merge=ours` driver, and complicates future merges. Always use `git merge`.

**`vcpkg.json` name field.**
The `"name"` field in `vcpkg.json` is cosmetic but should match the tool name.
It is easy to forget on first setup.

**`CMakeLists.txt` project name drives everything.**
The executable name, archive names, and CPack package name all derive from
`project(...)`. If you forget to change it, your tool builds an executable
named `cpp_app`.

**Merging frequently is easier than merging rarely.**
A merge after 2–3 template commits is trivial. A merge after 30 commits
involves more conflicts. Pull from the template whenever a meaningful batch
of improvements lands rather than letting it drift for months.

**The `template` remote survives `git clone`.**
If you clone your tool repo on a new machine, the `template` remote is not
there — it only exists in the original setup. After cloning a tool repo on a
new machine, re-add it:

```bash
git remote add template https://github.com/your-org/cpp-app
```

Consider documenting this in the tool repo's `README.md` under a
"Contributing / Dev Setup" section so you don't forget.
