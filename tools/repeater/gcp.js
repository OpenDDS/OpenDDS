const Compute = require('@google-cloud/compute')
const getLocalIps = require('./get-local-ips')

module.exports = (args, sendTo) => {
  const compute = new Compute()
  const gcpName = args.gcp.split(':', 2)
  const zone = compute.zone(gcpName[0])
  const ig = zone.instanceGroup(gcpName[1])
  let previousIps = []

  const updateFunc = () => {
    const localIps = getLocalIps()
    ig.getVMs().then((data) => {
      const vms = data[0]
      if (!vms || vms.length === 0) {
        console.log('No VMs in group')
        return
      }
      Promise.all(vms.map((vm) => vm.getMetadata())).then((responses) => {
        const ips = []
        for (let resp of responses) {
          const md = resp[0]
          for (let ni of md.networkInterfaces) {
            if (ni.networkIP && !localIps.includes(ni.networkIP)) {
              ips.push(ni.networkIP)
            }
          }
        }

        // Remove existing.
        previousIps.forEach((addr) => {
          delete sendTo[addr]
        })

        // Insert new.
        ips.forEach((addr) => {
          sendTo[addr] = args.uport
        })

        previousIps = ips
      })
    })
  }

  updateFunc()
  setInterval(updateFunc, args.interval * 1000)
}
