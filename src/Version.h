#ifndef VERSION_H
#define VERSION_H

#define VERSION_MAJOR		"1"
#define VERSION_MINOR		"2"
#define VERSION_RELEASE		"0"
#define VERSION_STRING		VERSION_MAJOR "." VERSION_MINOR "." VERSION_RELEASE

#define VERSION_REVISION_STRING	"$Revision$"
#define VERSION_REVISION_OFFSET	11
#define VERSION_REVISION_LENGTH	(sizeof(VERSION_REVISION_STRING) - (VERSION_REVISION_OFFSET + 3))
#define VERSION_REVISION_NUMBER	(VERSION_REVISION_STRING + VERSION_REVISION_OFFSET)

#endif
