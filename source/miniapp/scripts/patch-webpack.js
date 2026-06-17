/**
 * Removes "additionalProperties": false from webpack's ProgressPlugin JSON schema.
 *
 * webpackbar v5 stores { name, color, reporters, reporter } in this.options
 * (which it inherits from ProgressPlugin). webpack 5.106+ validates this.options
 * against ProgressPlugin.json — the restriction lives inside
 * definitions.ProgressPluginOptions.additionalProperties, not the top-level object.
 *
 * This script runs automatically as a postinstall hook and also before
 * `ray start --target web` is invoked by the IDE.
 */
'use strict';
const fs = require('fs');
const path = require('path');

const schemaPath = path.join(
  __dirname, '..', 'node_modules',
  'webpack', 'schemas', 'plugins', 'ProgressPlugin.json'
);

try {
  const schema = JSON.parse(fs.readFileSync(schemaPath, 'utf-8'));
  let patched = false;

  // The restriction is inside definitions.ProgressPluginOptions
  if (
    schema.definitions &&
    schema.definitions.ProgressPluginOptions &&
    schema.definitions.ProgressPluginOptions.additionalProperties === false
  ) {
    delete schema.definitions.ProgressPluginOptions.additionalProperties;
    patched = true;
  }

  // Also remove from the top level in case a future webpack version moves it
  if (schema.additionalProperties === false) {
    delete schema.additionalProperties;
    patched = true;
  }

  if (patched) {
    fs.writeFileSync(schemaPath, JSON.stringify(schema, null, 2) + '\n');
    console.log('patch-webpack: patched ProgressPlugin.json for webpackbar compatibility.');
  }
} catch (_) {
  // webpack not yet installed or schema missing — silently skip
}
