#!/usr/bin/env node
const dgram = require('dgram');
const args = require('yargs')
      .usage('$0: Multicast Datagram Repeater')
      .option('group', {
          describe: 'Multicast group (IPv4addr:port)',
          type: 'string',
          demandOption: true,
          requiresArg: true
      })
      .option('uport', {
          describe: 'Unicast port number',
          type: 'number',
          demandOption: true,
          requiresArg: true
      })
      .option('send', {
          describe: 'Adresses to send to',
          type: 'array',
          requiresArg: true
      })
      .option('gcp', {
          describe: 'Get send addresses from GCP (zone:group)',
          type: 'string',
          requiresArg: true
      })
      .option('verbose', {
          describe: 'Log each incoming datagram',
          alias: 'v',
          type: 'boolean'
      })
      .argv;

const mcast = args.group.split(':', 2);
const group = mcast[0];
const mport = mcast[1];

if (mport == args.uport) {
    throw new Error('Multicast port and unicast port must be distinct.');
}

const sendTo = {};
if (args.send) {
    args.send.forEach((addr) => {
        sendTo[addr] = 1;
    });
}

if (args.gcp) {
    require('./gcp.js')(args.gcp).then((addrs) => {
        addrs.forEach((addr) => {
            if (!sendTo[addr]) {
                console.log(`GCP: adding send address ${addr}`);
            }
            sendTo[addr] = 1;
        });
    });
}

const msock = dgram.createSocket({type: 'udp4', reuseAddr: true});
const usock = dgram.createSocket({type: 'udp4', reuseAddr: true});

msock.on('error', (err) => {
    console.log(`msock error:\n${err.stack}`);
    msock.close();
});

usock.on('error', (err) => {
    console.log(`usock error:\n${err.stack}`);
    usock.close();
});

msock.on('listening', () => {
    const address = msock.address();
    msock.addMembership(group);
    console.log(`listen on ${address.address}:${address.port} group ${group}`);
});

msock.on('message', (msg, rinfo) => {
    if (rinfo.port == args.uport) {
        return;
    }
    if (args.verbose) {
        console.log(`mc from ${rinfo.address}:${rinfo.port} ${rinfo.size} B`);
    }
    for (let sendAddr of Object.keys(sendTo)) {
        msock.send(msg, args.uport, sendAddr);
    }
});

usock.on('listening', () => {
    const address = usock.address();
    console.log(`listen on ${address.address}:${address.port} unicast`);
});

usock.on('message', (msg, rinfo) => {
    if (args.verbose) {
        console.log(`uc from ${rinfo.address}:${rinfo.port} ${rinfo.size} B`);
    }
    usock.send(msg, mport, group);
});

msock.bind(mport);
usock.bind(args.uport);
