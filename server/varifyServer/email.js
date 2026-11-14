const nodemailer = require('nodemailer');
const config = require('./config');

let transport;

function getTransport() {
  if (!transport) {
    transport = nodemailer.createTransport({
      pool: true,
      host: config.email.host,
      port: config.email.port,
      secure: config.email.secure,
      connectionTimeout: 10000,
      greetingTimeout: 10000,
      socketTimeout: 15000,
      disableFileAccess: true,
      disableUrlAccess: true,
      auth: {
        user: config.email.user,
        pass: config.email.password,
      },
    });
  }
  return transport;
}

async function sendVerificationCode(email, code) {
  if (!config.email.enabled) {
    console.log(`[development] verification code for ${email}: ${code}`);
    return;
  }

  const result = await getTransport().sendMail({
    from: `CPK Chat <${config.email.from || config.email.user}>`,
    to: email,
    subject: 'CPK Chat 注册验证码',
    text: `你的验证码是 ${code}，${config.redis.codeTtlSeconds} 秒内有效。请勿将验证码告诉他人。`,
    html: `<p>你的验证码是：</p><p style="font-size:24px;font-weight:bold">${code}</p><p>${config.redis.codeTtlSeconds} 秒内有效，请勿将验证码告诉他人。</p>`,
  });
  console.log(`Verification email accepted for ${email}, messageId=${result.messageId}`);
}

module.exports = { sendVerificationCode };
