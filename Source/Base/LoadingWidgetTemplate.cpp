// Copyright (c) MIT License

#include "LoadingWidgetTemplate.h"

FText ULoadingWidgetTemplate::GetDisplayText() const 
{
	return DisplayText;

}

/** Assigns passed FText to DisplayText */
void ULoadingWidgetTemplate::SetDisplayText(const FText& NewDisplayText) 
{
	DisplayText = NewDisplayText;
}