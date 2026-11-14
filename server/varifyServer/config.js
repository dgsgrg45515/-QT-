const fs = require('fs');
const path = require('path');

const configPath = path.join(__dirname, 'config.json');

function readConfig() {
  let fileConfig;
  try {
    fileConfig = JSON.parse(fs.readFileSync(configPath, 'utf8'));
  } catch (error) {
    throw new Error(`Unable to read ${configPath}: ${error.message}`);
  }

  const config = {
    server: {
      host: process.env.VERIFY_HOST || fileConfig.server?.host || '127.0.0.1',
      port: Number(process.env.VERIFY_PORT || fileConfig.server?.port || 50051),
    },
    redis: {
      host: process.env.REDIS_HOST || fileConfig.redis?.host || '127.0.0.1',
      port: Number(process.env.REDIS_PORT || fileConfig.redis?.port || 6379),
      password: process.env.REDIS_PASSWORD ?? fileConfig.redis?.password ?? '',
      database: Number(process.env.REDIS_DATABASE || fileConfig.redis?.database || 0),
      codeTtlSeconds: Number(
        process.env.CODE_TTL_SECONDS || fileConfig.redis?.codeTtlSeconds || 300,
      ),
    },
    email: {
      enabled: process.env.EMAIL_ENABLED
        ? process.env.EMAIL_ENABLED === 'true'
        : Boolean(fileConfig.email?.enabled),
      host: process.env.EMAIL_HOST || fileConfig.email?.host || 'smtp.163.com',
      port: Number(process.env.EMAIL_PORT || fileConfig.email?.port || 465),
      secure: process.env.EMAIL_SECURE
        ? process.env.EMAIL_SECURE === 'true'
        : fileConfig.email?.secure !== false,
      user: process.env.EMAIL_USER || fileConfig.email?.user || '',
      password: process.env.EMAIL_PASSWORD || fileConfig.email?.password || '',
      from: process.env.EMAIL_FROM || fileConfig.email?.from || fileConfig.email?.user || '',
    },
  };

  if (!Number.isInteger(config.server.port) || config.server.port < 1 || config.server.port > 65535) {
    throw new Error('server.port must be an integer between 1 and 65535');
  }
  if (!Number.isInteger(config.redis.port) || config.redis.port < 1 || config.redis.port > 65535) {
    throw new Error('redis.port must be an integer between 1 and 65535');
  }
  if (!Number.isInteger(config.redis.codeTtlSeconds) || config.redis.codeTtlSeconds < 1) {
    throw new Error('redis.codeTtlSeconds must be a positive integer');
  }
  if (config.email.enabled && (!config.email.user || !config.email.password)) {
    throw new Error('EMAIL_USER and EMAIL_PASSWORD are required when email is enabled');
  }

  return config;
}

module.exports = readConfig();
