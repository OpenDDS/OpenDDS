const os = require('os')

module.exports = () => {
  return Object.values(os.networkInterfaces())
    .reduce((acc, item) => acc.concat(item), [])
    .filter(addr => (addr.family === 'IPv4' && addr.internal === false))
    .map(addr => addr.address)
}
