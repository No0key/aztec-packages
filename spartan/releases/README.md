# Aztec Spartan

This tool helps easing the entry barrier to boot an Aztec Sequencer and Prover (S&P) Testnet.

![Aztec Sparta Meme](./assets/banner.jpeg)

For once, there's no rocket science here. This script does the following:

- Checks for the presence of Docker in your machine
- Prompts you for some environment variables
- Outputs a templated docker-compose file with your variables
- Runs the docker compose file

It should work in most UNIX-based machines.

## Installation

To configure a new node, create a new directory and run the install script:

```bash
mkdir val1 && cd val1
curl -L sp-testnet.aztec.network | bash
```

This will install `aztec-sequencer.sh` in the current directory. You can now run it:

```bash
./aztec-sequencer.sh config
```

If you don't have Docker installed, the script will do it for you. It will then prompt for any required environment variables and output both a `docker-compose.yml` and an `.env` file. You will also be prompted to choose whether to use a [named volume](https://docs.docker.com/engine/storage/volumes/) (default) or if you want to use a local directory to store the node's data.

Run `./aztec-sequencer.sh` without any command to see all available options, and pass them as flags, i.e. `./aztec-sequencer config -p 8080 -p2p 40400`. If you want to use a different key for p2p peer id, pass it with `-pk <your_key>`.

For more options, see the [Node Configuration](#node-configuration) section.

> [!TIP]
> Ensure that each validator instance uses unique ports to avoid conflicts.

## Running

To spare you a few keystrokes, you can use `./aztec-sequencer [start/stop/logs/update]` to start, stop, output logs or pull the latest docker images.

> [!NOTE]
> The above deploy script will connect your node to the p2p network where it will register peers and start receiving messages from other nodes on the network. You will not be in the validator set just yet.
>
> Once you connect and begin to see gossiped messages such as attestations, proposals etc notify notify a team member and they will add you to the validator set.

## Node Configuration

The user is prompted to enter some values which will map to corresponding ENV variables. Some are required:

1. A Sepolia execution node RPC (for example on [alchemy](https://dashboard.alchemy.com/))
2. A Sepolia beacon node RPC (for example from [drpc](https://drpc.org))
3. An Ethereum private key
4. `COINBASE` which is the Ethereum address associated with the private key

On a first run, the script will generate a p2p private key and store it in `$DATA_DIR/var/lib/aztec/p2p-private-key`. If you wish to change your p2p private key, you can pass it on as a CLI arg using the flag `-pk` or update the `PEER_ID_PRIVATE_KEY` in the env file.

### Publisher and Archiver

The Publisher is the main node component that interacts with the Ethereum L1, for read and write operations. It is mainly responsible for block publishing, proof submission and tx management.

The Archiver's primary functions are data storage and retrieval (i.e. L1->L2 messages), state synchronization and re-org handling.

| Variable                       | Description                                                                                                                                                          |
| ------------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| ETHEREUM_HOSTS                 | List of Ethereum nodes URLs your validator will connect to (comma separated). For as long as we're on private networks, please use the value in `aztec-sequencer.sh` |
| L1_CHAIN_ID                    | Chain ID of the L1                                                                                                                                                   |
| DATA_DIRECTORY                 | Optional dir to store archiver and world state data. If omitted will store in memory                                                                                 |
| ARCHIVER_POLLING_INTERVAL_MS   | The polling interval in ms for retrieving new L2 blocks and encrypted logs                                                                                           |
| SEQ_PUBLISHER_PRIVATE_KEY      | This should be the same as your validator private key                                                                                                                |
| SEQ_PUBLISH_RETRY_INTERVAL_MS  | The interval to wait between publish retries                                                                                                                         |
| SEQ_VIEM_POLLING_INTERVAL_TIME | The polling interval viem uses in ms                                                                                                                                 |

### Sequencer Config

The Sequencer Client is a criticial component that coordinates tx validation, L2 block creation, collecting attestations and block submission (through the Publisher).

| Variable                                   | Description                                                                                                                                                         |
| ------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| VALIDATOR_DISABLED                         | If this is True, the client won't perform any validator duties.                                                                                                     |
| VALIDATOR_ATTESTATIONS_POLLING_INTERVAL_MS | If not enough attestations, sleep for this long and check again                                                                                                     |
| GOVERNANCE_PROPOSER_PAYLOAD_ADDRESS        | To nominate proposals for voting, you must set this variable to the Ethereum address of the `proposal` payload. You must edit this to vote on a governance upgrade. |
| SEQ_ENFORCE_TIME_TABLE                     | Whether to enforce strict timeliness requirement when building blocks. Refer [here](#sequencer-timeliness-requirements) for more on the timetable                   |
| SEQ_MAX_TX_PER_BLOCK                       | Increase this to make larger blocks                                                                                                                                 |
| SEQ_MIN_TX_PER_BLOCK                       | Increase this to require making larger blocks                                                                                                                       |
| COINBASE                                   | This is the Ethereum address that will receive the validator's share of block rewards. It defaults to your validator address.                                       |
| FEE_RECIPIENT                              | This is the Aztec address that will receive the validator's share of transaction fees. Also defaults to your validator's address (but on Aztec L2).                 |

#### Sequencer Timeliness Requirements

During testing, it was helpful to constrain some actions of the sequencer based on the time passed into the slot. The time-aware sequencer can be told to do action A only if there's a certain amount of time left in the slot.

For example, at the beginning of a slot, the sequencer will first sync state, then request txs from peers then attempt to build a block, then collect attestations then publish to L1. You can create constraints of the form "Only attempt to build a block if under 5 seconds have passed in the slot".

If this is helpful in your testing as well, you can turn it on using the environment variable `SEQ_ENFORCE_TIME_TABLE`.

Currently the default timetable values are hardcoded in [sequencer.ts](https://github.com/AztecProtocol/aztec-packages/blob/master/yarn-project/sequencer-client/src/sequencer/sequencer.ts#L72). Time checks are enforced in `this.setState()`.

### P2P Config

The P2P client coordinates peer-to-peer communication between Nodes.

| Variable                    | Description                                                                                                                                                                                                                                                                          |
| --------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| BOOTSTRAP_NODES             | A list of bootstrap peer ENRs to connect to. Separated by commas.                                                                                                                                                                                                                    |
| P2P_IP                      | The client's public IP address. Defaults to working it out using disv5, otherwise set P2P_QUERY_FOR_IP if you are behind a NAT                                                                                                                                                       |
| P2P_PORT                    | The port that will be used for sending / receiving p2p messages. Defaults to 40400.                                                                                                                                                                                                  |
| P2P_LISTEN_ARR              | Address to listen on for p2p messages. Defaults to 0.0.0.0                                                                                                                                                                                                                           |
| P2P_QUERY_FOR_IP            | Useful in dynamic environments where your IP is not known in advance. Set this to True, and only supply `:TCP_PORT` and `:UDP_PORT` for the `ANNOUNCE_ADDR` variables. If you know your public IP address in advance, set this to False or just provide the full announce addresses. |
| P2P_ENABLED                 | Whether to run the P2P module. Defaults to False, so make sure to set to True                                                                                                                                                                                                        |
| P2P_MAX_PEERS               | The max number of peers to connect to.                                                                                                                                                                                                                                               |
| P2P_BLOCK_CHECK_INTERVAL_MS | How milliseconds to wait between each check for new L2 blocks.                                                                                                                                                                                                                       |

### Prover Config

Please refer to the Epoch Proving Integration [Guide](https://hackmd.io/@aztec-network/epoch-proving-integration-guide) for info on how to setup your prover node.

## Governance Upgrades

During a governance upgrade, we'll announce details on the discord. At some point we'll also write AZIPs (Aztec Improvement Proposals) and post them to either the github or forum to collect feedback.

We'll deploy the payload to the L1 and share the address of the payload with the sequencers on discord.

To participate in the governance vote, sequencers must change the variable `GOVERNANCE_PROPOSER_PAYLOAD_ADDRESS` in the Sequencer Client to vote during the L2 slot they've been assigned sequencer duties.

## Troubleshooting

> [!TIP]
> Please make sure you are in the Discord server and that you have been assigned the role `S&P Participant`. Say gm in the `sequencer-and-prover` channel and turn on notifications for the announcements channel.

If you encounter any errors or bugs, please try basic troubleshooting steps like restarting your node, checking ports and configs.

If issue persists, please share on the sequencer-and-prover channel and tag [Amin](discordapp.com/users/65773032211231539).

Some issues are fairly light, the group and ourselves can help you within 60 minutes. If the issue isn't resolved, please send more information:

**Error Logs**: Attach any relevant error logs. If possible, note the timestamp when the issue began.
**Error Description**: Briefly describe the issue. Include details like what you were doing when it started, and any unusual behaviors observed.
**Steps to Reproduce (if known)**: If there’s a clear way to reproduce the error, please describe it.
**System Information**: Share details like your system’s operating system, hardware specs, and any other relevant environment information.

That way we can dedicate more time to troubleshoot and open Github issues if no known fix.
