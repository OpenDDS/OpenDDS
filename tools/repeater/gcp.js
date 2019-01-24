const Compute = require('@google-cloud/compute')
const os = require('os')

module.exports = (args, sendTo) => {
  const compute = new Compute()
  const gcpName = args.gcp.split(':', 2)
  const zone = compute.zone(gcpName[0])
  const ig = zone.instanceGroup(gcpName[1])
  let previousIps = []

  const updateFunc = () => {
    ig.getVMs().then((data) => {
      const vms = data[0]
      if (!vms || vms.length === 0) {
        console.log('No VMs in group')
        return
      }
      const others = []
      for (let vm of vms) {
        if (vm.name !== os.hostname()) {
          others.push(vm)
        }
      }
      Promise.all(others.map((vm) => vm.getMetadata())).then((responses) => {
        const ips = []
        for (let resp of responses) {
          const md = resp[0]
          for (let ni of md.networkInterfaces) {
            if (ni.networkIP) {
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
