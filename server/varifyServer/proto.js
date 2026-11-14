const path = require('path');
const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');

const protoPath = path.join(__dirname, 'message.proto');
const packageDefinition = protoLoader.loadSync(protoPath, {
  keepCase: false,
  longs: String,
  enums: String,
  defaults: true,
  oneofs: true,
});

const descriptor = grpc.loadPackageDefinition(packageDefinition);

module.exports = {
  grpc,
  VarifyService: descriptor.message.VarifyService,
};
