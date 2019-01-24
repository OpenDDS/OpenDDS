#!/usr/bin/env node
const dgram = require('dgram')
const args = require('yargs')
  .usage('$0: Multicast Datagram Sender')
  .option('group', {
    describe: 'Multicast group (IPv4addr:port)',
    type: 'string',
    demandOption: true,
    requiresArg: true
  })
  .argv

const mcast = args.group.split(':')
if (mcast.length !== 2) {
  throw new Error('No multicast address and port.')
}

const group = mcast[0]
const mport = mcast[1]

const msock = dgram.createSocket({ type: 'udp4', reuseAddr: true })

msock.on('error', (err) => {
  console.log(`msock error:\n${err.stack}`)
  process.exit(1)
})

msock.send('Hello\n', mport, group)
setTimeout(() => {
  console.log('Sender exiting')
  process.exit(0)
}, 5 * 1000)
