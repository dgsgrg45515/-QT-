const Redis = require('ioredis');
const config = require('./config');

const redis = new Redis({
  host: config.redis.host,
  port: config.redis.port,
  password: config.redis.password || undefined,
  db: config.redis.database,
  lazyConnect: true,
  maxRetriesPerRequest: 1,
  connectTimeout: 3000,
});

redis.on('error', (error) => {
  console.error(`Redis error: ${error.message}`);
});

async function connectRedis() {
  if (redis.status === 'wait') {
    await redis.connect();
  }
  await redis.ping();
}

async function setVerificationCode(key, code, ttlSeconds) {
  await redis.set(key, code, 'EX', ttlSeconds);
}

async function deleteVerificationCode(key) {
  await redis.del(key);
}

async function closeRedis() {
  if (redis.status !== 'end') {
    await redis.quit();
  }
}

module.exports = {
  connectRedis,
  setVerificationCode,
  deleteVerificationCode,
  closeRedis,
};
