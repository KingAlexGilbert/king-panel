# Publish the first release

1. Copy this folder's contents into your existing local checkout, including
   `.github`, `.gitignore`, and `.gitattributes`. Keep your checkout's `.git` folder.
2. Review the diff, commit the updated files, and push your branch. The `dist`
   folder and generated EXEs are already ignored by Git.
3. Confirm the Build workflow succeeds and test the app on Windows.
4. From the intended release commit, create and push the `v1.0.0` tag:

   ```sh
   git tag v1.0.0
   git push origin v1.0.0
   ```

Pushing this tag triggers the existing Release workflow, which builds and
publishes the application, installer, and checksums automatically. It rejects a
tag that does not match `setup.nsi`. Do not move an existing published tag.

Alternatively, create a release manually through GitHub using the same tag and
upload the files from `dist`. Use one publishing route; the automated workflow
can replace assets with its own freshly compiled binaries when the tag is pushed.

Suggested title: **King Panel v1.0.0 - Initial release**

Suggested notes:

> A lightweight Windows tray utility for changing monitor refresh rates,
> resolutions, scaling, and supported primary-display HDR. Includes an installer
> with optional startup and shortcuts, plus a portable executable.
>
> Known issue: submenus may briefly appear downward before moving upward.
> Executables are unsigned.

## Checks performed for this package

- Application behavior matches the restored build; executable and installer
  version resources are updated to 1.0.0.
- Manifest and both required ICO resources are present.
- Version 1.0.0 binaries were rebuilt and checksummed; installer payload checked.
- Source cross-build checked with warnings treated as errors.
- Existing license, workflow files, and contribution guide were preserved.

Windows runtime tests and GitHub Actions execution were not available here.
No commit, tag, push, or GitHub release has been created by preparing this ZIP.
