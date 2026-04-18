const { spawnSync } = require('child_process');
const path = require('path');

const frontendDir = path.resolve(__dirname, '..');
const npxCmd = process.platform === 'win32' ? 'npx.cmd' : 'npx';
const supportedTargets = new Set(['--linux', '--mac', '--win']);
const providedTargets = process.argv.slice(2).filter((arg) => supportedTargets.has(arg));
const targets = providedTargets.length > 0 ? providedTargets : ['--linux'];

const result = spawnSync(npxCmd, ['electron-builder', ...targets], {
  cwd: frontendDir,
  stdio: 'inherit',
  env: {
    ...process.env,
    NODE_ENV: 'production'
  }
});

if (result.status !== 0) {
  process.exit(result.status || 1);
}
