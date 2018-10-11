![works on my machine](https://img.shields.io/badge/works-on%20my%20machine-green.png)

# htsfuse


FUSE (Filesystem in Userspace) (https://en.wikipedia.org/wiki/Filesystem_in_Userspace) accessing remote files behind http. 

It was first designed for bioinformatics but can be used for general purpose.

# Usage

```
./htsfuse site.xml -d DIRECTORY
```

# Compilation

The following packages must be installed:

  * libfuse-dev
  * libcurl-dev
  * libxml2-dev

e.g: 

```
$ sudo apt-get install libfuse-dev libcurl-dev libxml2-dev
```

Then run `make`:


```bash
$ make
```

# XML file

The XML file describe the virtual hierachy of the filesystem.

There is two type of nodes `<directory>` and `<file>`

The root of the XML file is always a `<directory>`

A `<directory>` (but the root node) *must* have an attribute `name` that will be used as the path component.

The root `<directory>` ignores the `name` (it is always `/`)

A `<directory>` contains any number of  `<directory>` and/or `<file>`

A  `<file>` *must* have an attribute `url` pointing to the remote URL.

A  `<file>` can have have an attribute `name` that will be used as the path component. 

```xml
<file name="file1.txt" url="http://ftp.1000genomes.ebi.ac.uk/vol1/ftp/current.tree"/>
```

If this attribute  `name`  is missing, the last part of the `url` will be used.

```xml
<file url="http://ftp.1000genomes.ebi.ac.uk/vol1/ftp/phase3/data/NA21099/alignment/NA21099.unmapped.ILLUMINA.bwa.GIH.low_coverage.20120522.bam"/>
```

## Authentication

A XML node can have two attributes `user` and `password` that will be used for Basic http Authentication. Those attributes are searched recursively in the parent nodes.

```xml
(...)
	<directory name="igvdata">
		<directory name="private" user="guest" password="password">
			<file url="http://data.broadinstitute.org/igvdata/private/SignalK562H3k36me3.tdf"/>
			<file url="http://data.broadinstitute.org/igvdata/private/cpgIslands.hg18.bed"/>
		</directory>
	</directory>
(...)
``




# Example

```bash 
# create the fuse directory
$ mkdir -p tmp_fuse
# invoke htsfuse
./htsfuse test.xml -d tmp_fuse 
```

in another terminal ...

```bash 
$ find tmp_fuse/ -type f 
tmp_fuse/testdata/file1.txt

tmp_fuse/testdata/NA21099.unmapped.ILLUMINA.bwa.GIH.low_coverage.20120522.bam
tmp_fuse/testdata/NA21099.unmapped.ILLUMINA.bwa.GIH.low_coverage.20120522.bam.bai
tmp_fuse/igvdata/private/SignalK562H3k36me3.tdf 
tmp_fuse/igvdata/private/cpgIslands.hg18.bed

$ ls -lah tmp_fuse/testdata/

total 0
drwxr-xr-x 2 root root   0 janv.  1  1970 .
drwxr-xr-x 2 root root   0 janv.  1  1970 ..
-r--r--r-- 1 root root 79M janv.  1  1970 file1.txt
-r--r--r-- 1 root root 29M janv.  1  1970 NA21099.unmapped.ILLUMINA.bwa.GIH.low_coverage.20120522.bam
-r--r--r-- 1 root root 30K janv.  1  1970 NA21099.unmapped.ILLUMINA.bwa.GIH.low_coverage.20120522.bam.bai

$ head tmp_fuse/igvdata/private/cpgIslands.hg18.bed

chr1	18598	19673	CpG:_116
chr1	124987	125426	CpG:_30
chr1	317653	318092	CpG:_29
chr1	427014	428027	CpG:_84
chr1	439136	440407	CpG:_99
chr1	523082	523977	CpG:_94
chr1	534601	536512	CpG:_171
chr1	703847	704410	CpG:_60
chr1	752279	753308	CpG:_115
chr1	778726	779074	CpG:_28


samtools view tmp_fuse/testdata/NA21099.unmapped.ILLUMINA.bwa.GIH.low_coverage.20120522.bam MT | tail
SRR360609.242026	77	MT	16477	37	96M1I3M	=	16477	0	GCTAAAGTGAACTGTATCCGACATCTGGTTCCTACTTCAGGGTCATAGAGCCTAAATAGCCCACACGTTCCCCTTAAATAAGACATCACGATGGATCACA	9FIGIKLKKKLIMLKKKLLCLJLKLNKMKKLLNLJNLLLLMMHLKLLMLMKLNLLMLLMLMLLJMKDJLLMMILHLMILICKHHIKHJD=FFEHBIFCCC	X0:i:1	X1:i:0	MD:Z:47A45C5	RG:Z:SRR360609	AM:i:0	NM:i:3	SM:i:37XT:A:U
SRR360609.242026	141	MT	16477	0	*	=	16477	0	TTCCCCTTAAATAAGATATCACGATGGATCACAGGTCTATCACCCTATTAACCACTCACGGGAGCTCTCCATGCATTTGGTATTTTCGTCTGGGGGGTGT	43602>D?@J&1.-3'/*9.;698CDJC?FIJFJGGFGACD?CFFJ4IDE?CE?C9LCCCMMDLLEMNKKMMILKMLKILLDJJGLHCJILJJIIIIHIH	RG:Z:SRR360609
SRR360549.15809473	77	MT	16478	37	95M1I4M	=	16478	0	CTAAAGTGAACTGTATCCGACATCTGGTTCCTACTTCAGGGTCATAGAGCCTAAATAGCCCACACGTTCCCCTTAAATAAGACATCACGATGGATCACAG	9HHIILKLLLIMLLLLKKCMIMLKHMMLKKLNLINLLMMMMIKMLIMIMLLNMMMJMFLLLNJMJDKLLKIMLLMMJKJHIKFKJILHCKLKIIIEDCFF	X0:i:1	X1:i:0	MD:Z:46A45C5T0	RG:Z:SRR360549	AM:i:0	NM:i:4	SM:i:37XT:A:U
SRR360549.15809473	141	MT	16478	0	*	=	16478	0	TGTGGCCCAAGGTCTGTCCCCCTATTAACCGCTCACGGGAGCTCTCCATGCATTTGGTATTTTCGTCTGGGGGGTGTGCACGCGATAGCATTGCGAGACG	%%%%%%%%%%%%%%%%%%%%7388D?<242(1?/3'7>53DA9:EACEEE+BHJJJCH:F@97255C?IIGEEEF?GE?=>9JAIHGEEGJDD8<HFF?:	RG:Z:SRR360549
SRR360549.6154536	77	MT	16478	37	95M1I4M	=	16478	0	CTAAAGTGAACTGTATCCGACATCTGGTTCCTACTTCAGGGTCATAGAGCCTAAATAGCCCACACGTTCCCCTTAAATAAGACATCACGATGGATCACAG	7B?AACE>FFA?EFEEED?H8GF;JEFGBAEFGBFBFFHFDFFBIHD@C9:GHIIH<B?,?E:D9AIG?:;4GF;IDDH@IJ?FIDJDAH5C@BDBG;BF	X0:i:1	X1:i:0	MD:Z:46A45C5T0	RG:Z:SRR360549	AM:i:0	NM:i:4	SM:i:37XT:A:U
SRR360549.6154536	141	MT	16478	0	55M45S	=	16478	0	TGGGATAGGGCAGGAATCAAAGACAGATACTGCGGCATAGGGTGCTCCGGCTCCAGCGGCTCGCAATGCCATCGCCCGCCCCACACCCCGACGAAAATAC	8B:<=1#>3/05:2'BF@0A:D8=09AE=3EC0:%?ACB7D=4?:I@B0E;EAA%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%	XC:i:55	RG:Z:SRR360549
SRR360609.4284672	77	MT	16481	0	*	=	16481	0	GCCTAAATAGCCCACACGTTCCCCTGTAATAAGACATCACGATGGATCNCAGGTCTATCACCCTATTAACCACTCACGGGAGCTCTCCATGCATTTGGTA	/71478=B;D56545-58A=(%#&B&/(-1:;8<58621453?>-:>6%9=>DE9@7@3+D;8KFIIGGE::9;8F9AHHIK9HDL=GFIJGGJHIHGHB	RG:Z:SRR360609
SRR360609.4284672	141	MT	16481	37	92M1I3M4S	=	16481	0	AAGTGAACTGTATCCGACATCTGGTTCCTACTTCAGGGTCATAGAGCCTAAATAGCCCACACGTTCCCCTTAAATAAGACATCACGATGGATCACAGGTC	7<E8EDBBHCCDE=EBFI:K89HHFBAAHC7JG9CDFFA@DH<FGMILGKCLL=ME7D74B><?=FEDDHI>ECGIDG9IGA3C>5<8@:=A;<4%%%%%	X0:i:1	X1:i:0	XC:i:96	MD:Z:43A45C5	RG:Z:SRR360609	AM:i:0	NM:i:3	SM:i:37	XT:A:U
SRR360612.13052334	77	MT	16503	0	*	=	16503	0	ACGATGGATCACAGGTCTATCACCCTATTAACCACTCACGGGAGCTCTCCATGCATTTGGTATTTTCGTCTGGGGGGTGTGCACGCGATAGCATTGCGAG	%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%D@;>;EE9D@F2E.*>DGD8GFHGCA6IHGI@=F?AD>DDFJJIJEG?E=9:GKFIGFJCIF?CF	RG:Z:SRR360612
SRR360612.13052334	141	MT	16503	37	70M30S	=	16503	0	GGTTCCTACTTCAGGGTCATAGAGCCTAAATAGCCCACACGTTCCCCTTAAATAAGACATCACGATGGATCACAGGCCTATCCCCCTATTACCCAATCAC	8>FF8C@CCI@7CEGKFK?=@A@D=?HDDD?ECEEA3;)2<A4@@EE9>A;DBD:?>?50956(=3@=<%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%	X0:i:1	X1:i:0	XC:i:70	MD:Z:21A45C2	RG:Z:SRR360612	AM:i:0	NM:i:2	SM:i:37	XT:A:U

```


# Author

Pierre Lindenbaum Phd / @yokofakun
Insitut du Thorax - Nantes - France




