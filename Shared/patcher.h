// C++
// Patcher Header

#ifndef PATCHER_H
#define PATCHER_H

#include <Windows.h>
#include <stdio.h>
#include <string.h>

class CPatcher
{
public:
	CPatcher(const void *, const void *, int);
	~CPatcher();

	bool Patch();
	bool Unpatch();

private:
	BYTE *m_PatchAddress;

	BYTE *m_szOriginalBytes;
	BYTE *m_szPatchedBytes;

	int m_nPatchLength;
};

#endif