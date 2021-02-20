// Copyright 2020 Markus Grech
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// 	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdio>
#include <exception>

#ifdef _WIN32
#define OVECTOR_WINDOWS
#endif

#ifdef OVECTOR_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <cerrno>
#include <cstring>

#include <sys/mman.h>
#endif

#include "ovector.hpp"

using namespace mgrech::detail;

#define OV_STRINGIFY2(x) #x
#define OV_STRINGIFY(x) OV_STRINGIFY2(x)
#define OV_HERE __FILE__ ":" OV_STRINGIFY(__LINE__)

namespace
{

constexpr size_type PAGE_SIZE = 4096;
constexpr size_type SIZE_TYPE_MAX = ~size_type();

OVECTOR_FORCE_INLINE
size_type ceil_multiple(size_type size, size_type n)
{
	return (size / n + (size % n != 0)) * n;
}

OVECTOR_FORCE_INLINE
bool add_overflows(size_type a, size_type b)
{
	return SIZE_TYPE_MAX - b < a;
}

int os_last_error();

[[noreturn]]
void fatal_error(char const* location, char const* message)
{
	// copy error out first
	auto error = os_last_error();
	std::fprintf(stderr, "%s: fatal error: %s: os error code %d\n", location, message, error);
	std::terminate();
}

#ifdef OVECTOR_WINDOWS

void* os_guarded_alloc(size_type dataSize, size_type guardSize)
{
	auto memory = VirtualAlloc(nullptr, dataSize + guardSize, MEM_RESERVE, PAGE_NOACCESS);

	if(!memory)
		return nullptr;

	if(!VirtualAlloc(memory, dataSize, MEM_COMMIT, PAGE_READWRITE))
		fatal_error(OV_HERE, "failed to commit allocation");

	return memory;
}

void os_dealloc(void* memory, size_type size)
{
	(void)size;

	if(!VirtualFree(memory, 0, MEM_RELEASE))
		fatal_error(OV_HERE, "failed to release memory");
}

int os_last_error()
{
	return (int)GetLastError();
}

#else

void* os_guarded_alloc(size_type dataSize, size_type guardSize)
{
	auto memory = mmap(nullptr, dataSize + guardSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if(memory == MAP_FAILED)
		return nullptr;

	if(mmap((char*)memory + dataSize, guardSize, PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) == MAP_FAILED)
		fatal_error(OV_HERE, "failed to enable guard page");

	return memory;
}

void os_dealloc(void* memory, size_type size)
{
	if(munmap(memory, size) == -1)
		fatal_error(OV_HERE, "failed to unmap memory");
}

int os_last_error()
{
	return errno;
}

#endif

} // namespace

void* mgrech::detail::guarded_alloc(size_type requestedDataSize, size_type requestedGuardSize)
{
	if(requestedDataSize == 0)
		return nullptr;

	// if rounding up to a page size multiple would overflow
	if(requestedDataSize > SIZE_TYPE_MAX - PAGE_SIZE + 1 || requestedGuardSize > SIZE_TYPE_MAX - PAGE_SIZE + 1)
		return nullptr;

	auto allocatedDataSize = ceil_multiple(requestedDataSize, PAGE_SIZE);
	auto allocatedGuardSize = ceil_multiple(requestedGuardSize, PAGE_SIZE);

	if(add_overflows(allocatedDataSize, allocatedGuardSize))
		return nullptr;

	auto memory = os_guarded_alloc(allocatedDataSize, allocatedGuardSize);

	if(!memory)
		return nullptr;

	auto wastedSpace = allocatedDataSize - requestedDataSize;
	return (char*)memory + wastedSpace;
}

void mgrech::detail::guarded_dealloc(void* memory, size_type requestedDataSize, size_type requestedGuardSize)
{
	auto allocatedDataSize = ceil_multiple(requestedDataSize, PAGE_SIZE);
	auto allocatedGuardSize = ceil_multiple(requestedGuardSize, PAGE_SIZE);
	auto wastedSpace = allocatedDataSize - requestedDataSize;
	auto allocatedMemory = (char*)memory - wastedSpace;
	os_dealloc(allocatedMemory, allocatedDataSize + allocatedGuardSize);
}
