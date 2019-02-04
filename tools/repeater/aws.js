const AWS = require('aws-sdk')
const ec2 = new AWS.EC2()
const getLocalIps = require('./get-local-ips')

module.exports = (args, sendTo) => {
  let previousIps = []

  const updateFunc = function (ips, nextToken) {
    const localIps = getLocalIps()
    const params = {
      DryRun: false,
      Filters: [
        {
          Name: 'tag-key',
          Values: [
            args.aws
          ]
        }
      ],
      MaxResults: 1000,
      NextToken: nextToken
    }
    ec2.describeInstances(params, function (err, data) {
      if (err) console.log(err, err.stack)
      else {
        data.Reservations.forEach(reservation => {
          reservation.Instances.forEach(instance => {
            instance.NetworkInterfaces.forEach(networkInterface => {
              networkInterface.PrivateIpAddresses.forEach(address => {
                if (!localIps.includes(address.PrivateIpAddress)) {
                  ips.push(address.PrivateIpAddress)
                }
              })
            })
          })
        })
        if (data.NextToken) {
          updateFunc(ips, data.NextToken)
        } else {
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
      }
    })
  }

  updateFunc([])
  setInterval(() => updateFunc([]), args.interval * 1000)
}
