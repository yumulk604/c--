#include "stdafx.h"
#include "MarkManager.h"

#ifdef __WIN32__
#include <direct.h>
#endif

#define OLD_MARK_INDEX_FILENAME "guild_mark.idx"
#define OLD_MARK_DATA_FILENAME "guild_mark.tga"

static Pixel * LoadOldGuildMarkImageFile()
{
	FILE * fp = fopen(OLD_MARK_DATA_FILENAME, "rb");

	if (!fp)
	{
		sys_err("cannot open %s", OLD_MARK_INDEX_FILENAME);
		return NULL;
	}

	int dataSize = 512 * 512 * sizeof(Pixel);
	Pixel * dataPtr = (Pixel *) malloc(dataSize);

	fread(dataPtr, dataSize, 1, fp);

	fclose(fp);

	return dataPtr;
}

bool GuildMarkConvert(const std::vector<DWORD> & vecGuildID)
{
	mkdir("mark", S_IRWXU);

	if (0 != access(OLD_MARK_INDEX_FILENAME, F_OK))
		return true;

	// 인덱스 파일 열기
	FILE* fp = fopen(OLD_MARK_INDEX_FILENAME, "r");

	if (NULL == fp)
		return false;

	// 이미지 파일 열기
	Pixel * oldImagePtr = LoadOldGuildMarkImageFile();

	if (NULL == oldImagePtr)
	{
		fclose(fp);
		return false;
	}

	/*
	// guild_mark.tga가 실제 targa 파일이 아니고, 512 * 512 * 4 크기의 raw 파일이다.
	// 눈으로 확인하기 위해 실제 targa 파일로 만든다.
	CGuildMarkImage * pkImage = new CGuildMarkImage;
	pkImage->Build("guild_mark_real.tga");
	pkImage->Load("guild_mark_real.tga");
	pkImage->PutData(0, 0, 512, 512, oldImagePtr);
	pkImage->Save("guild_mark_real.tga");
	*/
	sys_log(0, "Guild Mark Converting Start.");

	char line[256];
	DWORD guild_id;
	DWORD mark_id;
	Pixel mark[SGuildMark::SIZE];

	while (fgets(line, sizeof(line)-1, fp))
	{
		sscanf(line, "%u %u", &guild_id, &mark_id);

		if (find(vecGuildID.begin(), vecGuildID.end(), guild_id) == vecGuildID.end())
		{
			sys_log(0, "  skipping guild ID %u", guild_id);
			continue;
		}

		// mark id -> 이미지에서의 위치 찾기
		uint row = mark_id / 32;
		uint col = mark_id % 32;

		if (row >= 42)
		{
			sys_err("invalid mark_id %u", mark_id);
			continue;
		}

		uint sx = col * 16;
		uint sy = row * 12;

		Pixel * src = oldImagePtr + sy * 512 + sx;
		Pixel * dst = mark;

		// 옛날 이미지에서 마크 한개 복사
		for (int y = 0; y != SGuildMark::HEIGHT; ++y)
		{
			for (int x = 0; x != SGuildMark::WIDTH; ++x)
				*(dst++) = *(src+x);

			src += 512;
		}

		// 새 길드 마크 시스템에 넣는다.
		CGuildMarkManager::instance().SaveMark(guild_id, (BYTE *) mark);
		line[0] = '\0';
	}

	free(oldImagePtr);
	fclose(fp);

	system("mv -f guild_mark.idx guild_mark.idx.removable");
	system("mv -f guild_mark.tga guild_mark.tga.removable");

	sys_log(0, "Guild Mark Converting Complete.");

	return true;
}

