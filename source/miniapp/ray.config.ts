import * as path from 'path';

const config = {
  resolveAlias: {
    '@pages': path.resolve(__dirname, 'src', 'pages'),
  },
  plugins: [
    {
      name: 'fix-web-target',
      configWebpack({ config }: { config: any }) {
        // The route loader generates `import('@pages/home/index')` from
        // path:'pages/home/index'. webpack's '@' alias only matches '@/'
        // (with slash), so '@pages/...' is treated as an npm namespace.
        config.resolve.alias.merge({
          '@pages': path.resolve(__dirname, 'src', 'pages'),
        });
      },
    },
  ],
};

module.exports = config;
