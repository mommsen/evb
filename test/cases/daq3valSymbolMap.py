data['LAUNCHER_BASE_PORT'] = 17777
data['SOAP_BASE_PORT'] = 25000
data['I2O_BASE_PORT'] = 54320
data['FRL_BASE_PORT'] = 55320

dataNetwork = ".d3vrbs1v0.cms"

hosts = [
#    "d3vrubu-c2e33-08-01", # EVM

    "d3vrubu-c2e34-06-01",
    "d3vrubu-c2e34-08-01",
    "d3vrubu-c2e34-10-01",
    "d3vrubu-c2e34-12-01",
    "d3vrubu-c2e34-14-01",
    "d3vrubu-c2e34-16-01",
    "d3vrubu-c2e34-18-01",
    "d3vrubu-c2e34-20-01",
    "d3vrubu-c2e34-27-01",
    "d3vrubu-c2e34-29-01",
    "d3vrubu-c2e34-31-01",
    "d3vrubu-c2e34-33-01",
    "d3vrubu-c2e34-35-01",
    "d3vrubu-c2e34-37-01",
    "d3vrubu-c2e34-39-01",
    "d3vrubu-c2e34-41-01",

    "d3vrubu-c2e33-06-01",
]

# add RUs and BUs
ruIndex = 0
buIndex = 0

for host in hosts:

    if host in ("d3vrubu-c2e34-18-01"):
#    if host in ("d3vrubu-c2e34-20-01"):
#    if host in (""):
        data['BU%d_SOAP_HOST_NAME' % buIndex] = host + ".cms"
        data['BU%d_I2O_HOST_NAME' % buIndex] = host + dataNetwork
        buIndex += 1
    else:
        data['RU%d_SOAP_HOST_NAME' % ruIndex] = host + ".cms"
        data['RU%d_I2O_HOST_NAME' % ruIndex] = host + dataNetwork
        ruIndex += 1
