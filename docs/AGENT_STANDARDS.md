# Agent Standards

This document defines the standard set of companion markdown files that every
software repository should maintain. It is written for AI coding agents, but is
equally useful for human engineers onboarding to a new project.

"Companion files" are markdown files that live alongside source code. They are
distinct from end-user product documentation (manuals, PDFs, help systems). Their
audience is people and agents who *work on* the codebase — building, extending,
and maintaining it.

---

## The Core Companion Files

Every repository following this standard maintains at least three companion files
at the repository root. These are the *starting points* — each may link out to
additional files (sub-architecture docs, per-module READMEs, deeper guides), but
these three are always present and always current:

| File | Primary audience | Purpose |
|---|---|---|
| `README.md` | Anyone who finds the repo | What it is, how to build it, how to use it |
| `ARCHITECTURE.md` | Engineers and agents working on the code | Why it is built the way it is |
| `AGENTS.md` | AI coding agents (and human contributors) | Rules and constraints that must not be violated |

These files are the single source of truth for their respective concerns. When
they conflict with each other or with the code, the code wins — but the files
must then be updated to match.

---

## `README.md` — The Front Door

**Audience:** Anyone — a developer evaluating the project, a new team member, a
CI system, or an end user building from source.

**Purpose:** Answer the question *"what is this and how do I use it?"* without
requiring the reader to open any source file.

**Typical contents:**
- What the project does (one paragraph, jargon-free)
- Supported platforms and minimum versions
- One-time setup: every tool that must be installed, with a detect step and an
  install hint for each
- Build instructions for every supported platform
- How to run the application
- How to run the tests
- How to trigger CI manually or understand CI results

**Rules:**
- Setup steps must be written so a developer on a clean machine can follow them
  top to bottom without prior knowledge. Do not assume tools are already present.
- Every tool entry must have a *detect* step (a command that confirms the right
  version is installed) and an *install* hint. The detect step must verify the
  desired state, not just existence — it should state the minimum required version.
- Installation hints should point to a stable URL or package manager command.
  Do not document every click in an installer wizard; those change with every release.
- Do not describe internal design decisions here. Those belong in `ARCHITECTURE.md`.

---

## `ARCHITECTURE.md` — The Engineering Record

**Audience:** Engineers and AI agents who need to understand *why* the code is
structured the way it is before making changes.

**Purpose:** Record the decisions, constraints, and rationale behind the
implementation. A reader should finish this file knowing why the project uses the
libraries it does, why certain design patterns were chosen, and what the hard
constraints are that must not be violated.

**Typical contents:**
- Repository structure overview (directory tree with one-line descriptions)
- Dependency manifest: every third-party library, what it does, and why it was
  chosen over alternatives
- Key design decisions with explicit rationale ("we chose X over Y because…")
- Platform-specific constraints and how they are handled
- What is intentionally *not* supported, and why
- CI/CD pipeline description: what jobs run, when, and what they verify

**Rules:**
- Focus on *why*, not *what*. The source code already shows what. This file
  explains the reasoning a reader cannot infer from the code alone.
- When a decision has a non-obvious consequence (e.g. a minimum OS version, a
  flag that must not be removed), document it here so future engineers and agents
  do not accidentally undo it.
- Keep it accurate. An outdated architecture document is worse than none — it
  misleads agents and engineers into making wrong assumptions.

---

## `AGENTS.md` — The Rulebook

**Audience:** AI coding agents primarily, but equally applicable to any human
contributor. The rules here are project invariants — things no one should violate
regardless of how the code is being written. Naming this file `AGENTS.md` ensures
AI tools that automatically look for agent instruction files (GitHub Copilot,
Claude, Cursor, etc.) will find and respect it.

**Purpose:** Tell agents what they must and must not do when working on this
specific repository. This file contains project-specific rules that override or
supplement any general agent behaviour.

**Typical contents:**
- A mandatory rule requiring agents to keep all three companion files up to date
- Core design principles that must never be violated (e.g. "all binaries must be
  statically linked")
- Platform-specific constraints the agent must respect
- Coding conventions (naming, file structure, patterns to use or avoid)
- Rules specific to the build system, dependency manager, or CI pipeline

**Rules:**
- This is a *constraints* document, not a *tutorial*. It should be terse and
  direct. Agents read it to know what they are not allowed to do, not to learn
  how the project works.
- The single most important rule to include: **agents must update all three
  companion files whenever they make a change that affects what those files
  describe.** This rule must be stated explicitly, prominently, and without
  exceptions.

---

## The Mandatory Update Rule

Every `AGENTS.md` file written under this standard must contain a variant of the
following rule, prominently placed:

> Any change to source code, build configuration, CI workflows, or any other
> file in the repository triggers a documentation check. Before committing,
> verify that all companion files still accurately describe the repository.
> This includes every file listed in the mandatory update section of `AGENTS.md`,
> **and any file referenced or linked from those files.** Update whichever files
> are now inaccurate.

This rule exists because AI agents are fast and thorough at implementing changes
but will skip documentation updates unless explicitly required to make them. The
rule must be non-optional, must list all companion files by name, and must
explicitly extend to any files those companions link to.

---

## Conflict Resolution: User Instructions vs. Companion Files

When an agent receives a user instruction that conflicts with a rule in a companion
file, it must not silently comply with the instruction or silently update the file.
The correct procedure is:

1. **Halt and surface the conflict.** Tell the user which rule is being violated and
   in which companion file it appears.
2. **Ask the user to choose** one of three outcomes:
   - **Comply with the file:** The user adjusts their request to align with the
     existing rule. No companion file changes needed.
   - **Update the file:** The rule should change. Update the companion file to reflect
     the new direction before or alongside the implementation.
   - **Intentional exception:** This one change deliberately deviates from the rule.
     The user must provide the reason. The agent adds a comment at the point of the
     change — in whatever comment syntax the file supports (source comment, CMake
     comment, YAML comment, etc.) — stating that it is intentionally non-compliant,
     which rule it violates, and the stated reason. The companion file is **not**
     updated; the rule remains in force for all other cases.

This procedure prevents two failure modes: agents that blindly execute conflicting
instructions (silently eroding standards), and agents that silently rewrite standards
to match whatever the user asks without surfacing the tension.

Every `AGENTS.md` written under this standard should include a version of this
conflict-resolution procedure, listing the specific companion files it applies to.

---

## Permitted Operations

An `AGENTS.md` file may explicitly grant permission for certain operations so that
the agent does not interrupt the user's workflow with unnecessary confirmation
requests. Permissions should be specific and scoped — granting broad permissions
(e.g. "do anything without asking") undermines the purpose of the standard.

**Recommended permissions to consider granting explicitly:**

- **Anonymous web reads** — fetching public URLs for reference material, docs lookup,
  or standards verification. Should exclude authenticated requests and writes to
  external services.
- **Read-only filesystem access** — reading files and directories without modification.
- **Creating new files or folders** — adding files that do not yet exist. This is
  generally safe to grant because no existing content is modified or deleted.

Operations that are not explicitly permitted should be performed with clear
communicated intent, even if they don't require confirmation. Irreversible operations
(deleting files, pushing to remotes, modifying existing files outside the scope
described by the user) should never be assumed to be permitted.

---

## Referencing This Standard

A repository following this standard should include the following line near the
top of its `AGENTS.md`:

> This repository follows the agent standards defined in `AGENT_STANDARDS.md`.

When this standard is published to a standalone repository, that line should
reference the URL instead:

> This repository follows the agent standards defined at `https://…/AGENT_STANDARDS.md`.
