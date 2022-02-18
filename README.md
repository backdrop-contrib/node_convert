# Node Convert

This module allows to convert one or many nodes between different node types.
It can convert the most popular field types: text, link, image, file, entity
reference, node / term / user reference.

The module provides:
- an API for converting nodes
- hooks for executing additional behaviors on conversion
- integrates with hook_node_operations and Backdrop's Action API
- integration with Rules

## Installation

- Install this module using the [official Backdrop CMS instructions](https://backdropcms.org/guide/modules).

## Configuration and Usage

Basic instructions are below. More details may be found (or added) in the [Wiki](https://github.com/backdrop-contrib/node_convert/wiki)

### Single node conversion:
1) Set 'administer conversion' and 'convert to x', 'convert from y' permissions.
2) Go to **node/x/convert** and follow the provided steps to convert the node.

### Multiple node conversion:
1) Set appropriate permissions.
2) Go to **admin/structure/node_convert_templates**
3) Create a new template following the the provided steps.
4) Go to **admin/content**
5) Select the relevant nodes.
6) Choose "Convert content" from the update options and click "Execute"
7) Select the conversion template you wish to use.
8) Click Update.

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
