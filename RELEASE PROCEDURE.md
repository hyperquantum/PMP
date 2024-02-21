# Release procedure

## 1. Preparation

- make sure all important features are done
- make sure no high priority bugs are left
- create screenshot(s)
- complete the changelog
- rewrite and clean up things in the changelog

## 2. Testing

- extensive testing of the server
- test all clients
- test the script for creating a release with a complete build from scratch

## 3. Release

1. create release commit
2. add release tag to that commit
3. run release script on that tagged commit
4. create release in GitHub: add changelog and binaries
5. create next-version commit
6. update downloads section on the website
7. announce the release
