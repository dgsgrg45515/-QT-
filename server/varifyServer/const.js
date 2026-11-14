const CODE_PREFIX = 'code_';

const Errors = Object.freeze({
  Success: 0,
  RedisErr: 1,
  Exception: 2,
  InvalidEmail: 3,
  EmailErr: 4,
});

module.exports = { CODE_PREFIX, Errors };
