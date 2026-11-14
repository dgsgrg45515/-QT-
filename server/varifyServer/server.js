const crypto = require('crypto');

const config = require('./config');
const { CODE_PREFIX, Errors } = require('./const');
const { sendVerificationCode } = require('./email');
const { grpc, VarifyService } = require('./proto');
const {
  connectRedis,
  setVerificationCode,
  deleteVerificationCode,
  closeRedis,
} = require('./redis');

const EMAIL_PATTERN = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

async function getVarifyCode(call, callback) {
  const email = String(call.request.email || '').trim();
  if (!EMAIL_PATTERN.test(email)) {
    callback(null, { error: Errors.InvalidEmail, email, code: '' });
    return;
  }

  const code = crypto.randomInt(100000, 1000000).toString();
  const key = `${CODE_PREFIX}${email}`;

  try {
    await setVerificationCode(key, code, config.redis.codeTtlSeconds);
  } catch (error) {
    console.error(`Unable to store verification code: ${error.message}`);
    callback(null, { error: Errors.RedisErr, email, code: '' });
    return;
  }

  try {
    await sendVerificationCode(email, code);
    callback(null, { error: Errors.Success, email, code: '' });
  } catch (error) {
    console.error(`Unable to send verification email: ${error.message}`);
    await deleteVerificationCode(key).catch(() => {});
    callback(null, { error: Errors.EmailErr, email, code: '' });
  }
}

async function startServer() {
  await connectRedis();

  const server = new grpc.Server();
  server.addService(VarifyService.service, { getVarifyCode });
  const address = `${config.server.host}:${config.server.port}`;

  await new Promise((resolve, reject) => {
    server.bindAsync(address, grpc.ServerCredentials.createInsecure(), (error, port) => {
      if (error) {
        reject(error);
        return;
      }
      console.log(`VerifyServer listening on ${config.server.host}:${port}`);
      resolve();
    });
  });

  const shutdown = () => {
    server.tryShutdown(async () => {
      await closeRedis().catch(() => {});
      process.exit(0);
    });
  };
  process.once('SIGINT', shutdown);
  process.once('SIGTERM', shutdown);
}

startServer().catch((error) => {
  console.error(`VerifyServer failed to start: ${error.stack || error.message}`);
  process.exit(1);
});
