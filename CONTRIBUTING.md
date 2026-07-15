# Contributing

This document will guide you through setting up your local environment to start developing for this project. It also covers
the steps needed to get a [hacked retail PS Vita](https://vita.hacks.guide/) ready for development.

## Bug Reports

To report bugs simply open a new GitHub Issue for this project. It's most helpful if you include steps to reproduce the issue
and if you can attach some screenshots to it.

## Setup Local Development Environment

This project is built using the Vita SDK. You must have it installed before developing for this project. For more information
about that go here: https://vitasdk.org/

Once you have that installed you can reference the [GitHub Actions YAML files](./.github/) to see what you need to build the
project on macOS or Linux. If you're using Windows it's
[recommended to use WSL](https://learn.microsoft.com/en-us/windows/wsl/install).

## Setup Development PS Vita

This project's [Makefile](Makefile) is designed to quickly deploy new development builds to a hardware PS Vita running
**vitacompanion** and **Cat-A-Log**.

### vitacompanion

[vitacompanion](https://github.com/devnoname120/vitacompanion) provides an always-on command and FTP server that this project
uses to:

- Upload new builds to the PS Vita
- Download logs and screenshots from the PS Vita
- Reboot the PS Vita

To install vitacompanion on the PS Vita:

1. Download **vitacompanion.suprx** from: https://github.com/devnoname120/vitacompanion/releases/latest
2. Upload the file to the PS Vita into the directory: **ur0:tai/**
3. Add this to **ur0:tai/config.txt**:
    ```text
    *main
    ur0:tai/vitacompanion.suprx
    ```
4. Reboot the PS Vita
5. Get the PS Vita's IP address: Settings > Network > Internet Connection Status

#### Verify

> [!NOTE]
> The Makefile will first move all screenshots on the PS Vita out of their random subdirectories and into the parent
> directory. In other words the SCREENSHOT directory structure will be flattened.

Take a screenshot (PS Button + Start) and then run (replace 192.168.0.100 with your PS Vita's IP address):

```bash
make PSVITA_IP=192.168.0.100 fetch-screenshots
ls fetch-screenshots
```

For more commands, run:

```bash
make help
```

### Cat-A-Log

[Cat-A-Log](https://github.com/isage/catlog) is used to read logs live from the PS Vita in your development machine's
terminal.

To install Cat-A-Log on the PS Vita:

1. Download **catlog.suprx** and **catlog.skprx** from: https://github.com/isage/catlog/releases/latest
2. Upload both files to the PS Vita into the directory: **ur0:tai/**
3. Add this to **ur0:tai/config.txt**:
    ```text
    *KERNEL
    ur0:tai/catlog.skprx
    *NPXS10015
    ur0:tai/catlog.suprx
    ```
4. Reboot the PS Vita
5. Get your computer's IP address, then input it in: Settings > Network > Cat Log > Log host
    1. Set **Log port** to: 10224
    2. Then return to main Settings menu
6. In your local terminal run `make recv-logs`, you should instantly see a lot of logs appear

> [!TIP]
> If you don't see any logs, try rebooting the PS Vita.
> If you still don't see any logs, make sure you've set the right IP address and that you can ping the PS Vita from your
> computer.

### Deploy

After you've setup **vitacompanion** and **Cat-A-Log** try deploying this project to the PS Vita.

1. Add this to **ur0:tai/config.txt**:
    ```
    *KERNEL
    ur0:tai/vitaQmBluetooth.skprx
    *main
    ur0:tai/QuickMenuReborn.suprx
    ```
2. Run this command:
    ```bash
    make PSVITA_IP=192.168.0.100 deploy-init deploy reboot recv-logs
    ```

## Releases

These are the steps a maintainer will take to make a new release.

1. Create a new pull request with the following changes:
    1. Finalize the [CHANGELOG.md](CHANGELOG.md) file and resetting the **Unreleased** section to "N/A".
    2. Set the new version in the main [`CMakeLists.txt`](CMakeLists.txt#L28) file. <!-- MINOR-YML -->
2. After merging the PR, run the release workflow:
    1. https://github.com/Robpol86/vitaQmBluetooth/actions/workflows/release.yml
    1. Click on the **Run workflow** dropdown in the blue bar.
    1. Leave the branch as `main`, set the new version using the `X.X.X` convention, then click on the green **Run workflow**
       button.
3. Wait for workflow to run, then check the Releases page.
    1. Confirm the new release has all of the assets uploaded to it.
    2. Confirm the new release body has the install instructions and release changelog.
