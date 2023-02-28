/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef CODECONTAINEROPTIONS_H
#define CODECONTAINEROPTIONS_H


#include <String.h>


enum format_capability {
	CAP_AUDIO_VIDEO,
	CAP_AUDIO_ONLY
};


class ContainerOption {
public:
					ContainerOption(const BString& option, const BString& extension,
						const BString& description, format_capability capability);

	BString 		Option;
	BString 		Extension;
	BString 		Description;
	format_capability Capability;
};


class CodecOption {
public:
					CodecOption(const BString& option, const BString& shortlabel,
						const BString& description);

	BString 		Option;
	BString 		Shortlabel;
	BString 		Description;
};



#endif // CODECONTAINEROPTIONS_H
