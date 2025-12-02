void CRC_BuildTable();

DWORD CRC_Calculate(DWORD crc, void* buffer, DWORD count);
DWORD CRC_CalculatePalette(DWORD crc, void* buffer, DWORD count);
