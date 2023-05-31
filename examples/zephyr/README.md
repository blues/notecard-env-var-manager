# Zephyr Example

A simple example showing how to use the Notecard Environment Variable Manager (NEVM) with Zephyr.

## `main.c`

This file contains the example code. Before building it, you'll first need to uncomment this line:

```c
/* #define PRODUCT_UID "com.your-company:your-product-name" */
```

and replace `com.your-company:your-product-name` with [the ProductUID](https://dev.blues.io/notehub/notehub-walkthrough/#finding-a-productuid) from a [Notehub](https://dev.blues.io/notehub/notehub-walkthrough/) project.

The `main.c` code sends a request to Notehub every 20 seconds for these environment variables:

- `variable_a`
- `variable_b`
- `variable_c`

## Notehub

Navigate to your [Notehub project](https://notehub.io/projects), click the Devices tab, double-click your device, and open the Environment tab. Under "Device environment variables", set a value for each variable and click Save:

![Settings variables on Notehub](../images/setting_vars_on_notehub.png "Settings variables on Notehub")

## Hardware

We'll be using [the Swan](https://dev.blues.io/swan/introduction-to-swan/) as our target MCU. You will also need

- Micro USB cable
- Notecard
- Cellular antenna
- Notecarrier F

You can get the Notecard, antenna, and Notecarrier F all in one bundle with the [Blues Starter Kit](https://shop.blues.io/collections/blues-starter-kits). Follow the [Swan Quickstart](https://dev.blues.io/quickstart/swan-quickstart) to assemble your hardware.

## Firmware

From the command line, you'll need to pull in the [note-c](https://github.com/blues/note-c) and [note-zephyr](https://github.com/blues/note-zephyr) submodules that the firmware depends on (these commands assume you're in the `zephyr` directory of this repository):

```sh
$ git submodule update --init note-c
$ git submodule update --init note-zephyr
```

To build and flash the firmware, you'll need:

* [Visual Studio Code (VS Code)](https://code.visualstudio.com/).
* [Docker and the VS Code Dev Containers extension](https://code.visualstudio.com/docs/devcontainers/containers). The Dev Containers documentation will take you through the process of installing both Docker and the extension for VS Code.

These instructions will defer parts of the build process to the [Blues Zephyr SDK documentation](https://dev.blues.io/tools-and-sdks/firmware-libraries/zephyr-sdk) (the "Zephyr SDK docs"). Though these instructions are for the [note-zephyr repo](https://github.com/blues/note-zephyr), the same patterns for building the code are used here.

1. Start VS Code and select File > Open Folder and pick `notecard-env-var-manager/examples/zephyr`.
1. Follow the instructions for your OS in the [Zephyr SDK docs' "Building the Dev Container" section](https://dev.blues.io/tools-and-sdks/firmware-libraries/zephyr-sdk/#building-the-dev-container).
1. Follow the [Zephyr SDK docs' "Building and Running" section](https://dev.blues.io/tools-and-sdks/firmware-libraries/zephyr-sdk/#building-and-running).

Now, the code should be running on the Swan. To view serial logs or debug the code, check out the [Zephyr SDK docs' "Debugging" section](https://dev.blues.io/tools-and-sdks/firmware-libraries/zephyr-sdk/#debugging). When viewing the serial logs, you should see output like this:

```
Fetch interval lapsed. Fetching environment variables...
{"req":"env.get","names":["variable_a","variable_b","variable_c"]}
{"body":{"variable_a":"Blues","variable_b":"is","variable_c":"cool!"}}
variable_a has value Blues
variable_b has value is
variable_c has value cool!
```

If you change the value of one of the variables on Notehub, you should see that reflected in the serial output shortly:

```
Fetch interval lapsed. Fetching environment variables...
{"req":"env.get","names":["variable_a","variable_b","variable_c"]}
{"body":{"variable_a":"IoT","variable_b":"is","variable_c":"cool!"}}
variable_a has value IoT
variable_b has value is
variable_c has value cool!
```
