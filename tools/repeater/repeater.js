#!/usr/bin/env node
const dgram = require('dgram')
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
  .option('interval', {
    describe: 'Polling interval in seconds for GCP',
    type: 'number',
    requiresArg: true,
    default: 60
  })
  .option('verbose', {
    describe: 'Log each incoming datagram',
    alias: 'v',
    type: 'boolean',
    default: false
  })
  .argv

const mcast = args.group.split(':')
if (mcast.length !== 2) {
  throw new Error('No multicast address and port.')
}

const group = mcast[0]
const mport = mcast[1]

if (mport === args.uport) {
  throw new Error('Multicast port and unicast port must be distinct.')
}

const sendTo = {}
if (args.send) {
  args.send.forEach((addr) => {
    const pieces = addr.split(':')
    if (pieces.length === 2) {
      sendTo[pieces[0]] = pieces[1]
    } else {
      sendTo[addr] = args.uport
    }
  })
}

if (args.gcp) {
  require('./gcp.js')(args, sendTo)
}

const msock = dgram.createSocket({ type: 'udp4', reuseAddr: true })
const usock = dgram.createSocket({ type: 'udp4', reuseAddr: true })

msock.on('error', (err) => {
  console.log(`msock error:\n${err.stack}`)
  msock.close()
})

usock.on('error', (err) => {
  console.log(`usock error:\n${err.stack}`)
  usock.close()
})

msock.on('listening', () => {
  const address = msock.address()
  msock.addMembership(group)
  console.log(`listen on ${address.address}:${address.port} group ${group}`)
})

msock.on('message', (msg, rinfo) => {
  if (rinfo.port === args.uport) {
    return
  }
  if (args.verbose) {
    console.log(`mc from ${rinfo.address}:${rinfo.port} ${rinfo.size} B`)
  }
  for (let sendAddr of Object.keys(sendTo)) {
    msock.send(msg, sendTo[sendAddr], sendAddr)
  }
})

usock.on('listening', () => {
  const address = usock.address()
  console.log(`listen on ${address.address}:${address.port} unicast`)
})

usock.on('message', (msg, rinfo) => {
  if (args.verbose) {
    console.log(`uc from ${rinfo.address}:${rinfo.port} ${rinfo.size} B`)
  }
  usock.send(msg, mport, group)
})

msock.bind(mport)
usock.bind(args.uport)
