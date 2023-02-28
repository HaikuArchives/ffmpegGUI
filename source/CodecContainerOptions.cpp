/*
 * Copyright 2023. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Andi Machovec (BlueSky), andi.machovec@gmail.com
*/


#include "CodecContainerOptions.h"


ContainerOption::ContainerOption(const BString& option, const BString& extension,
	const BString& description, format_capability capability)
	:
	Option(option),
	Extension(extension),
	Description(description),
	Capability(capability)
{
}


CodecOption::CodecOption(const BString& option,const BString& shortlabel,const BString& description)
	:
	Option(option),
	Shortlabel(shortlabel),
	Description(description)
{
}
