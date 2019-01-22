const MsRest = require('ms-rest-azure');
const NetworkManagementClient = require('azure-arm-network');
const os = require('os');

function getLocalIps() {
    return Object.values(os.networkInterfaces())
        .reduce((acc, item) => acc.concat(item), [])
        .filter(addr => ('IPv4' === addr.family && addr.internal === false))
        .map(addr => addr.address)
}

if (!process.env['AZURE_SUBSCRIPTION_ID']) {
  throw new Error('Please set the AZURE_SUBSCRIPTION_ID environment variable')
}
const subscriptionId = process.env['AZURE_SUBSCRIPTION_ID'];

module.exports = (args, sendTo) => {
  let previousIps = []

  const updateSendTo = function(ips) {
    // Remove existing.
    previousIps.forEach((addr) => {
      delete sendTo[addr]
    })

    // Insert new.
    ips.forEach((addr) => {
      sendTo[addr] = args.uport
    })

    previousIps = ips
  }

  const processInterfaces = function(client, interfaces, ips, localIps) {
    interfaces.forEach(interface => {
      if (interface.virtualMachine) {
        interface.ipConfigurations.forEach(config => {
          if (!localIps.includes(config.privateIPAddress)) {
            ips.push(config.privateIPAddress);
          }
        });
      }
    });
    if (interfaces.nextLink) {
      client.networkInterfaces.listNext(interfaces.nextLink).then(interfaces => {
        processInterfaces(client, interfaces, ips, localIps);
      });
    } else {
      updateSendTo(ips)
    }
  }

  const updateFunc = () => {
    MsRest.loginWithVmMSI().then(credentials => {
      let client = new NetworkManagementClient(credentials, subscriptionId);
      const ips = [];
      client.networkInterfaces.list(args.azure).then(interfaces => {
        processInterfaces(client, interfaces, ips, getLocalIps());
      });
    });
  }

  updateFunc()
  setInterval(updateFunc, args.interval * 1000)
}
