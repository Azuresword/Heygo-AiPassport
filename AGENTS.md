[简体中文](AGENTS.zh_CN.md) · **English**

# Project communication rules

These rules apply to all work in the Heygo AI Passport project.

## Required skill

Use the [asd-ste100 skill](https://github.com/danyuchn/asd-ste100-skill) for every project message and document.
This includes progress reports, final responses, agent instructions, code comments, commit messages, and project documents.

Read the installed `asd-ste100/SKILL.md` before you write.
If the skill is missing, install it from the source above.

## Language

- Communicate with the user in Chinese unless the user requests another language.
- Write each default Markdown document in English.
- Provide a matching Simplified Chinese document with the `.zh_CN.md` suffix.
- Keep both documents consistent. Add reciprocal language links.

The skill defines rules for English.
Apply its clarity principles to Chinese: short sentences, explicit actions, and consistent terms.
Do not describe Chinese text as compliant with an official STE standard.

## Writing rules

- Use Strict mode for procedures, error messages, tool descriptions, and agent instructions.
- Use STE-flavored mode for READMEs, explanatory text, and other documents covered by that mode.
- Use active voice and simple tenses where they preserve the meaning.
- Give one instruction per sentence.
- Limit English instructions to 20 words per sentence. Limit English descriptions to 25 words per sentence.
- Keep one topic per paragraph. Limit each paragraph to six sentences.
- Use lists for three or more steps or conditions.
- Use the same term for the same concept. Define necessary technical terms.
- Avoid semicolons, phrasal verbs, and unsupported claims about quality.
- Preserve facts, numbers, conditions, exceptions, and uncertainty when you rewrite text.
- If a shorter sentence would change the meaning, keep the necessary wording and explain the exception.

## Checks

Run the skill's `scripts/ste-lint.py` on English documents that you create or change.
Read the text after the check.
The check does not prove that a rewrite preserves meaning or uses the official ASD dictionary.
Check Chinese text manually.
