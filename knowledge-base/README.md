# XRD RAG Knowledge Base — Profex AI Agent Reference

This directory contains structured reference material for AI agents working with
Profex/BGMN for XRD analysis. The content is designed for RAG (Retrieval-Augmented
Generation) workflows — AI assistants can index these markdown files for semantic
search during refinement sessions.

## Available References

| File | Content |
|------|---------|
| `bgmn-parameters.md` | BGMN parameter reference (.par file format) |
| `rietveld-guidelines.md` | Best practices for Rietveld refinement |
| `phase-identification.md` | Search-Match and phase identification guide |

## Usage with AI Agents

Point your AI agent to this directory for XRD-specific knowledge:

```python
# Example: loading RAG context
import glob
rag_context = ""
for f in glob.glob("knowledge-base/*.md"):
    with open(f) as fh:
        rag_context += f"\n\n---\n# From: {f.name}\n" + fh.read()
```
