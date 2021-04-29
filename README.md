# Node Convert

This module allows to convert one or many nodes between different node types.
It can convert the most popular field types: text, link, image, file, entity
reference, node / term / user reference.

The module provides:
- an API for converting nodes
- hooks for executing additional behaviors on conversion
- integrates with hook_node_operations and Drupal's Action API
- integration with Rules

## Installation

- Install this module using the [official Backdrop CMS instructions](https://backdropcms.org/guide/modules).

## Configuration and Usage

Basic instructions are below. More details may be found (or added) in the [Wiki](https://github.com/backdrop-contrib/node_convert/wiki)

### Single node conversion:
1) Set 'administer conversion' and 'convert to x', 'convert from y' permissions.
2) Go to /node/x/convert and follow the provided steps to convert the node.

### Multiple node conversion (using hook_node_operations)
1) Set appropriate permissions.
2) Go to **admin/structure/node_convert_templates**
3) Create a new template following the the provided steps.
4) Go to **admin/content**
5) Select the relevant nodes.
6) Choose "Convert template: x" (based on the template name created above) from the update options.
7) Click Update.

### Multiple node conversion (using Actions API + Views Bulk Operations)
*Note: This requires the contributed modules Views and Views Bulk Operations*

1) Set appropriate permissions.
2) Go to **admin/structure/node_convert_templates**
3) Create a new template following the the provided steps (also check Create Action).
4) Create a new view with the options you require.
5) Add Views Bulk Operations under fields.
6) Configure all options of Bulk Operations as necessary.
7) Select "Convert a node" as an operation or an action that was created together with the conversion template.
   Note that VBO Operations are gathered from two sources:
  - Backdrop core actions (hook_action_info() and advanced actions added through the Actions UI)
  - Rules
8) Save the view. View it.
9) Select the necessary nodes and click the Convert x button.

## Issues

Bugs and Feature requests should be reported in the [Issue Queue](https://github.com/backdrop-contrib/node_convert/issues)

## Current Maintainers

- [Laryn Kragt Bakker](https://github.com/laryn), [CEDC.org](https://CEDC.org)
- Collaboration and co-maintainers welcome!

## Credits

- Ported to Backdrop by [Laryn Kragt Bakker](https://github.com/laryn), [CEDC.org](https://CEDC.org)
- Maintained for Drupal by [alcroito](https://www.drupal.org/u/alcroito)

## License

This project is GPL-2.0 (or later) software. See the LICENSE.txt file in this directory for
complete text.
