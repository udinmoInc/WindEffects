# Security Policy

## Supported Versions

WindEffects Engine is in early development. Security updates are provided for the current development branch only.

## Reporting a Vulnerability

If you discover a security vulnerability, report it responsibly.

### How to Report

**Do not** open a public issue for security vulnerabilities.

Send your report to:
- **Email**: security@udinmo.com
- **PGP Key**: Available upon request

### What to Include

- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Proof-of-concept code or screenshots (if applicable)
- Suggested fix or mitigation (if known)

### Response Timeline

- Acknowledgment within 48 hours
- Detailed response within 7 days, including:
  - Confirmation of the vulnerability
  - Estimated timeline for a fix
  - Coordination on disclosure timing

### Disclosure Policy

- We will work with you to understand and fix the vulnerability
- Credit in the security advisory (unless you prefer anonymity)
- Coordinated public disclosure timeline
- No disclosure before a fix is available

## Security Best Practices

### For Developers

- Keep dependencies updated
- Follow secure coding practices
- Validate and sanitize inputs
- Implement proper error handling
- Use secure authentication and authorization
- Keep sensitive data encrypted
- Follow the principle of least privilege

### For Users

- Keep your engine version updated
- Use strong authentication credentials
- Review third-party dependencies
- Monitor security advisories
- Report suspicious activity

## Security Features

| Feature | Status |
|---------|--------|
| Memory Safety (C++23) | Implemented |
| Input Validation | Implemented |
| Asset Security (signed/verified loading) | Implemented |
| Network Security | Planned |
| Sandboxing | Planned |

## Dependency Security

- Vulnerability scanning of dependencies
- Regular security updates
- License review for dependencies
- Minimal external dependencies

## Security Advisories

Published on GitHub Security Advisories with:
- CVE identifier (if applicable)
- CVSS severity score
- Affected versions
- Fixed versions
- Mitigation steps
- Reporter credit

## Contact

For security questions not related to vulnerability reports:
- **Email**: security@udinmo.com
- **PGP Key**: Available upon request