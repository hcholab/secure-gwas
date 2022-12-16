## About

This package is a fork of the repo [here](https://github.com/hhcho/secure-gwas),
which contains the client software for an end-to-end
multiparty computation (MPC) protocol for secure GWAS as
described in:

    Secure genome-wide association analysis using multiparty computation
    Hyunghoon Cho, David J. Wu, Bonnie Berger
    Nature Biotechnology, 2018

This package modifies the original code to support practical usage in an online setting.

## Compilation

First, update the paths in code/Makefile:

    CPP points to the clang++ compiler executable.
    INCPATHS points to the header files of installed libraries.
    LDPATH contains the .a files of installed libraries.

To compile, run ./make inside the code/ directory.

This will create three executables, 2 of which are of interest:

    bin/DataSharingClient
    bin/GwasClient

This process should take only a few seconds.

## How to run

Our MPC protocol consists of three entities: CP0, CP1, and CP2.
CP1 and CP2 are study participants and provide input data to the protocol,
and CP0 does some precomputation to help CP1 and CP2 securely carry out GWAS.

An instance of the client program is created for each involved
party on different machines, where the ID of the corresponding
party is provided as an input argument: 0=CP0, 1=CP1, 2=CP2.

These multiple instances of the client will interact over the
network to jointly carry out the MPC protocol.

For testing purposes, some (or all) of the instances may be run
on the same machine.

==== Step 1: Setup Parameters ====

We provide example parameter settings in:

    par/test.par.PARTY_ID.txt

For more information about each parameter, please consult code/param.h
and Supplementary Information of the original publication.

For a test run, update the following parameters according to your
network environment and leave the rest:

    PORT_*
    IP_ADDR_*

Make sure that the specified ports are not blocked by the firewall.

==== Step 2: Setup Input Data ====

We provide an example data set in
test_data1/ (for CP1) and test_data2/ (for CP2) directories.

==== Step 3: Initial Data Sharing ====

On the respective machines, cd into code/ and run DataSharingClient
for each party in the following order:

    CP0: bin/DataSharingClient 0 ../par/test.par.0.txt
    CP1: bin/DataSharingClient 1 ../par/test.par.1.txt ../test_data1/
    CP2: bin/DataSharingClient 2 ../par/test.par.2.txt ../test_data2/

During this step, CP1 and CP2 securely share their data.
The resulting shares are stored in the cache/ directory.

For the toy dataset, this process should take only a few seconds.

==== Step 4: GWAS ====

On the respective machines, cd into code/ and run GwasClient
for each party (excluding SP) in the following order:

    CP0: bin/GwasClient 0 ../par/test.par.0.txt
    CP1: bin/GwasClient 1 ../par/test.par.1.txt
    CP2: bin/GwasClient 2 ../par/test.par.2.txt

The final output including the association statistics ("assoc") and
the QC filter results for individuals ("ikeep") and SNPs ("gkeep1",
"gkeep2") are provided in the output/ directory.

Note these files are created only on the machine where CP2 is running.

This step also takes only a few seconds on the toy dataset. Expected
runtimes for realistic dataset sizes can be found in our manuscript.

### Contact for questions:

For questions about this fork of the repo, feel free to contact me at
Simon Mendelsohn, smendels@broadinstitute.org. If you have questions about the
original repo, you can contact Hoon Cho, hhcho@mit.edu.
