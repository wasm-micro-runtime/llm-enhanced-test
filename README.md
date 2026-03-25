# llm-enhanced-test

[agents](./agents/) contains all agents used to generate WAMR test cases with LLMs. If you modify existing agents or add new ones, keep the current directory structure.

### How to use these agents

The `agents` directory includes two parts:

1. Agents for generating new test cases.
2. Agents for reviewing and fixing generated test cases (`tests-pipeline.md`, `tests-review.md`, `tests-verify.md`, `tests-fix.md`).

When using these agents, copy [AGENTS.md](./agents/AGENTS.md), [CLAUDE.md](./agents/CLAUDE.md), [.claude](./agents/.claude), and [.kode](./agents/.kode) to the WAMR repository root so the agents can better understand the project structure.

Copy scripts under [scripts](./agents/scripts) into the `tests/unit` directory, which is the agents' working directory.

For example, to run the `tests-pipeline` agent with Claude Code CLI:

```shell
claude --dangerously-skip-permissions < path/to/agents/.claude/agents/tests-pipeline.md
```

In Claude, using `--dangerously-skip-permissions` bypasses all permission checks. **Make sure you run it only in a secure environment**.
