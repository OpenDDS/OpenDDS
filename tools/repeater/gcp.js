const Compute = require('@google-cloud/compute');
const os = require('os');

module.exports = (zoneAndGroup) => {
    const compute = new Compute();
    const gcpName = zoneAndGroup.split(':', 2);
    const zone = compute.zone(gcpName[0]);
    const ig = zone.instanceGroup(gcpName[1]);
    return ig.getVMs().then((data) => {
        const vms = data[0];
        if (!vms || vms.length == 0) {
            throw new Error('No VMs in group');
        }
        const others = [];
        for (let vm of vms) {
            if (vm.name != os.hostname()) {
                others.push(vm);
            }
        }
        return Promise.all(others.map(vm => vm.getMetadata())).then((responses) => {
            const ips = [];
            for (let resp of responses) {
                const md = resp[0];
                for (let ni of md.networkInterfaces) {
                    if (ni.networkIP) {
                        ips.push(ni.networkIP);
                    }
                }
            }
            return ips;
        });
    });
}
