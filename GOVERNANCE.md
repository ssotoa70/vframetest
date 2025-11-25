# Project Governance

This document describes how decisions are made in the vframetest project.

## Decision-Making Framework

The vframetest project uses a consensus-based decision-making model with clear escalation paths for disagreements.

### Decision Levels

#### Level 1: Routine Decisions
**Scope**: Bug fixes, documentation updates, minor improvements

**Authority**: Single maintainer
**Process**: Standard code review and approval
**Timeline**: Fast track (hours to days)
**Reversible**: Yes

Examples:
- Documentation updates
- Bug fixes with clear solutions
- Minor code cleanup
- Test additions

#### Level 2: Standard Changes
**Scope**: New features, performance improvements, refactoring

**Authority**: Two maintainers
**Process**: Detailed code review, testing, community feedback
**Timeline**: Normal (days to weeks)
**Reversible**: Yes

Examples:
- New filesystem detection features
- Performance optimizations
- Error handling improvements
- API enhancements

#### Level 3: Major Decisions
**Scope**: Architecture changes, breaking changes, release policy, platform support

**Authority**: All maintainers + community discussion
**Process**: GitHub issues/discussions, consensus building, RFC process
**Timeline**: Extended (weeks to months)
**Reversible**: Usually not (or with significant effort)

Examples:
- API redesign
- Platform support decisions
- License changes
- Release strategy changes
- Adding/removing major features

## Feature Request Process

1. **Discussion Phase** (Optional)
   - Open issue for community feedback
   - Discuss use cases and requirements

2. **Proposal Phase**
   - Detailed description of feature
   - Implementation approach
   - API/interface design
   - Testing strategy

3. **Review Phase**
   - Technical review by maintainers
   - Design feedback and iteration
   - Community input period (1-2 weeks)

4. **Implementation Phase**
   - Code development
   - Testing and documentation
   - Code review process

5. **Integration Phase**
   - Merge to main
   - Inclusion in next release

## Issue Triage

### Reporting Issues

All issues should include:
- Clear title and description
- Steps to reproduce (for bugs)
- Expected vs actual behavior
- Environment information (OS, version, etc.)

### Labeling System

- **bug**: Reported defect
- **enhancement**: Feature request
- **documentation**: Doc updates needed
- **help wanted**: Need community assistance
- **good first issue**: Suitable for new contributors
- **phase-2**: Part of Phase 2 roadmap
- **phase-3**: Part of Phase 3 roadmap

### Resolution Timeline

- **Bug Fixes**: Best effort, priority based on severity
- **Feature Requests**: Reviewed within 1-2 weeks
- **Documentation**: No SLA, community-driven
- **Questions**: Answered within a few days

## Release Schedule

vframetest follows an **irregular, feature-driven release cycle**:

- **Releases**: When features/fixes are ready and tested
- **No fixed schedule**: Driven by community needs and contributions
- **Announcement**: Via GitHub releases and README
- **Version**: Semantic versioning (MAJOR.MINOR.PATCH)

### Release Decision Criteria

A release is considered when:
- One or more significant features are complete
- All critical bugs are fixed
- Documentation is updated
- All tests pass
- Code quality standards met

## Maintainer Consensus

Decisions involving multiple maintainers use consensus:

1. **Discussion**: Present proposal and reasoning
2. **Feedback**: Collect input and address concerns
3. **Iteration**: Refine based on feedback
4. **Agreement**: Aim for consensus
5. **Decision**: If no consensus, maintainer lead decides

## Community Input

While the maintainer team makes final decisions, community input is valued:

- Open issue discussion welcome for all proposals
- Community feedback considered in feature decisions
- Large features may have "Request for Comments" (RFC) phase
- Users encouraged to voice concerns and suggestions

## Code Review Standards

All code contributions must pass:

1. **Compilation**: No warnings with `-Wall -Werror -Wpedantic`
2. **Testing**: Appropriate test coverage
3. **Documentation**: Updated docs and comments
4. **Code Style**: Consistent with project style
5. **Performance**: No regressions
6. **Security**: No vulnerabilities or risks

## Dispute Resolution

If disagreement arises:

1. **Discussion**: Respectful dialogue to understand different perspectives
2. **Documentation**: Record positions and reasoning
3. **Escalation**: If needed, request third-party perspective
4. **Decision**: Maintainer lead makes final decision
5. **Closure**: Decision documented and communicated

## Transparency

The project maintains transparency through:

- Open issue discussions
- Public pull requests with visible reviews
- Release notes explaining changes
- GitHub Projects for roadmap visibility
- Documented decision rationale

## Feedback Mechanisms

Community can provide feedback through:

- **GitHub Issues**: Bug reports, feature requests
- **GitHub Discussions**: General discussions (if enabled)
- **Pull Requests**: Code review and suggestions
- **Email**: For sensitive matters (ssotoa70@gmail.com)

## Long-term Vision

The vframetest project is committed to:

1. **Stability**: Maintaining backwards compatibility when possible
2. **Quality**: High code and documentation standards
3. **Community**: Supporting user needs and contributions
4. **Innovation**: Advancing storage testing capabilities
5. **Accessibility**: Easy to use for professionals and researchers

## Amendments

This governance document may be updated as the project evolves:

- Proposed amendments discussed openly
- Community feedback considered
- Changes documented with rationale
- Existing decision records maintained

---

**Last Updated**: November 25, 2025
**Version**: 1.0
**Status**: Active
