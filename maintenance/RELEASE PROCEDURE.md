# Release procedure

## 1. Preparation

- make sure all important features are done
- make sure no high priority bugs are left
- fix all build warnings (including GitHub build)
- create screenshot(s)
- complete the changelog
- rewrite and clean up things in the changelog

## 2. Testing

- extensive testing of the server
- test all clients
- test the script for creating a release with a complete build from scratch

## 3. Release

### 1. Create release commit
  * update README: point to new screenshot
  * update CHANGELOG: replace "unreleased" with the version number and the date
  * update PMPVersion.cmake: adjust version numbers
### 2. Add release tag to that commit
### 3. Run release script on that tagged commit
### 4. Create release in GitHub: add changelog and binaries
  * title: PMP x.y.z
  * copy changelog information from CHANGELOG.md
  * add binaries (zip file) created by the release script
### 5. Create next-version commit
  * add placeholders for unreleased changes
### 6. Update downloads section on the website
### 7. Announce the release
