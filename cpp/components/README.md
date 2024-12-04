# Components

This directory contains all the custom components written to support the projects in this repository.

The projects are included by relative path symlinks. Ideally, we would change this to instead publish the components as stand-along consumables that could be versioned, published, and consumed by the various projects.

## Include a component

To include one of these components in a project,

1. The project must be located in this repository
2. Navigate to the project directory you wish to include a components in.
3. Create a symlink in the projects components directory pointing to a component directory here
  `ln -s ../../../components/BleAdvertiser BleAdvertiser`

### Storing the symlink in Git

Ideally the component files of the symlink are not stored and the symlink itself is instead stored. This can be accomplished with the `core.symlinks` setting.

Get the current setting:
`git config --get core.symlinks`

Set the setting:
`git config --global core.symlinks true`

## Include a component in another repo

TODO: It's certainly possible, but probably just copy and paste for now

## Creating a new component here

These steps haven't been tested

1. Copy an existing component
2. Modify the CMakeLists.txt
3. Change the filenames accordingly