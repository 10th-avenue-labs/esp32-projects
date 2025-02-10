# Components

This directory contains all the custom components written to support the projects in this repository.

The projects are included by relative path symlinks. Ideally, we would change this to instead publish the components as stand-along consumables that could be versioned, published, and consumed by the various projects.

## Include a component

To include one of these components in a project,

1. The project must be located in this repository
2. Navigate to the project directory you wish to include a components in, for example, [waiter in the component-tests directory](../component-tests/waiter/).
   `cd ../component-tests/Waiter/`
3. Create the components directory if necessary. `mkdir components`
4. Navicage to the components directory in the project.
5. Create a symlink in the projects components directory pointing to a component directory here
   `ln -s ../../../components/Waiter components/Waiter`
   1. Some components require other components too, which would need to also be symlinked to be included. Build will fail otherwise.
6. Run build `esp.if build`. Note: A fullclean may be required first.
7. Include the component with a `#include "myComponent.h"`. No other changes are necessary.
8. Begin using and testing the component

### Storing the symlink in Git

Ideally the component files of the symlink are not stored and the symlink itself is instead stored. This can be accomplished with the `core.symlinks` setting.

Get the current setting:
`git config --get core.symlinks`

Set the setting:
`git config --global core.symlinks true`

## Include a component in another repo

TODO: It's certainly possible, but probably just copy and paste for now

## Creating a New Component

1. Copy an existing component directory, such as [Result](./Result/) for example.
   `cp -r /Result mynewComponentName`
2. Modify the CMakeLists.txt
3. Change the filenames accordingly
4. Modify the component headers for `#ifndef`. Example `#ifndef RESULT_H` to `#ifndef WAITER_H` in `Waiter.h`.
5. Some components require certain features to be enabled. For example Result requires runtime-type information. Check the components readme to find more information for these components with additional requirements.
6. Use the component in your project to test/develop it, (instructions on using above)
