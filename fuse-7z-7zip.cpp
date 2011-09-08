
#include "fuse-7z.hpp"
#include "fuse-7z-node.hpp"

#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <sstream>

#if defined(USE_P7ZIP)

#include "CPP/7zip/Archive/IArchive.h"
#include "CPP/7zip/Common/FileStreams.h"
#include "CPP/7zip/ICoder.h"
#include "CPP/7zip/IPassword.h"
#include "CPP/7zip/MyVersion.h"
#include "CPP/7zip/PropID.h"
#include "CPP/Common/MyCom.h"
#include "CPP/Common/MyWindows.h"
#include "CPP/Windows/PropVariant.h"



	//pimp = new Fuse7z_pimp(archiveName);
	// TODO init pimp
	
	unsigned int numItems = 0;
	//pimp->archive->GetItemCount(&numItems);

	logger << "Archive " << archiveName << " has " << numItems << " items" << Logger::endl;

	int nid = 0;
	for (int i = 0; i < numItems; i++) {

		std::string filename;
		//pimp->archive->GetStringProperty(i, kpidPath, &filename);
		//
		Node * node = root_node->insert(const_cast<char*>(filename.c_str()));
		node->id = i;

		struct stat & st = node->stat;
		st.st_ino = nid++;

		unsigned long long fsize;
		//arch_item->GetUInt64Property(kpidPhySize, fsize);
		st.st_size = fsize;
		unsigned long long mtime, atime, ctime;
		//arch_item->GetFileTimeProperty(kpidATime, atime);
		//arch_item->GetFileTimeProperty(kpidCTime, ctime);
		//arch_item->GetFileTimeProperty(kpidMTime, mtime);
		st.st_atime = atime;
		st.st_ctime = ctime;
		st.st_mtime = mtime;
	}



class C7ZipArchive;
class C7ZipOutStreamWrap;
class C7ZipInStream;
class C7ZipOutStream;
class CInFileStreamWrap;

class C7ZipInStream
{
public:
	virtual wstring GetExt() const = 0;
	virtual int Read(void *data, unsigned int size, unsigned int *processedSize) = 0;
	virtual int Seek(long long int offset, unsigned int seekOrigin, unsigned long long int *newPosition) = 0;
	virtual int GetSize(unsigned long long int * size) = 0;
};


class C7ZipArchive
{
	public:
	C7ZipArchive();
	virtual ~C7ZipArchive();

	public:
	virtual bool GetItemCount(unsigned int * pNumItems) {
#if 0
		*pNumItems = (unsigned int)m_ArchiveItems.size();
#endif
	}
	virtual bool Extract(unsigned int index, C7ZipOutStream * pOutStream) {
#if 0
		CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback(pOutStream);
		CMyComPtr<IArchiveExtractCallback> extractCallback(extractCallbackSpec);
		return m_pInArchive->Extract(&index, 1, false, extractCallbackSpec) == S_OK;
#endif
	}

	void Initialize() {
#if 0
		UInt32 numItems = 0;

		if (m_pInArchive->GetNumberOfItems(&numItems) != 0) {
			stringstream ss;
			ss << "Could not get number of items in archive";
			throw runtime_error(ss.str());
		}

		for(UInt32 i = 0; i < numItems; i++)
		{
			C7ZipArchiveItem * pItem = NULL;

			if (Create7ZipArchiveItem(this, m_pInArchive, i, &pItem))
			{
				m_ArchiveItems.push_back(pItem);
			}
		}
#endif
	}

	void Close() {
#if 0
		m_pInArchive->Close();
#endif
	}
#if 0
	void GetUInt64Property(int propertyIndex, unsigned long long & val) {

		NWindows::NCOM::CPropVariant prop;

		if (m_pInArchive->GetArchiveProperty(p7zip_index, &prop) != 0)
			return false;

		if (prop.vt == VT_UI8 || prop.vt == VT_UI4) {
			val = ConvertPropVariantToUInt64(prop);
			return true;
		}

		return false;


	}
	void GetBoolProperty(int propertyIndex, bool & val) const {
		switch(propertyIndex) {
			case kpidSolid:
			case kpidEncrypted:
			break;

			default:
			{
				stringstream ss;
				ss << "Property index " << propertyIndex << " is not a boolean";
				throw runtime_error(ss.str());
			}
			break;
		}

		NWindows::NCOM::CPropVariant prop;

		if (m_pInArchive->GetArchiveProperty(p7zip_index, &prop) == 0 && prop.vt == VT_BOOL) {
			val = prop.bVal;
		}
		else {
			stringstream ss;
			ss << "Could not get property index " << propertyIndex;
			throw runtime_error(ss.str());
		}

	}

	void GetStringProperty(int propertyIndex, string & val) const {

		switch(propertyIndex) {
			case kpidComment:
			case kpidCharacts:
			case kpidCreatorApp:
			case kpidVolumeName:
			case kpidPath:
			case kpidUser:
			case kpidGroup:
			break;
			default:
			{
				stringstream ss;
				ss << "Property index " << propertyIndex << " is not a boolean";
				throw runtime_error(ss.str());
			}
			break;

		}

		NWindows::NCOM::CPropVariant prop;

		if (!m_pInArchive->GetArchiveProperty(p7zip_index, &prop) && prop.vt == VT_BSTR) {
			val = prop.bstrVal;
		}
		else {
			stringstream ss;
			ss << "Could not get property index " << propertyIndex;
			throw runtime_error(ss.str());
		}
	}
		
	void GetFileTimeProperty(int propertyIndex, unsigned long long & val) const {

		switch(propertyIndex) {
			case kpidCTime:
			case kpidATime:
			case kpidMTime:
			break;
			default:
			{
				stringstream ss;
				ss << "Property index " << propertyIndex << " is not a boolean";
				throw runtime_error(ss.str());
			}
			break;
		}

		NWindows::NCOM::CPropVariant prop;

		if (m_pInArchive->GetArchiveProperty(p7zip_index, &prop) == 0 && prop.vt == VT_FILETIME) {
			unsigned __int64 tmp_val = 0;
			memmove(&tmp_val, &prop.filetime, sizeof(unsigned __int64));
			val = tmp_val;
			return true;
		}
		else {
			stringstream ss;
			ss << "Could not get property index " << propertyIndex;
			throw runtime_error(ss.str());
		}
	}
#endif
};


#if 0

class C7ZipOutStream
{
public:
	virtual int Write(const void *data, unsigned int size, unsigned int *processedSize) = 0;
	virtual int Seek(long long int offset, unsigned int seekOrigin, unsigned long long int *newPosition) = 0;
	virtual int SetSize(unsigned long long int size) = 0;
};


class C7ZipOutStreamWrap:
	public IOutStream,
	public CMyUnknownImp
{
public:
	C7ZipOutStreamWrap(C7ZipOutStream * pOutStream) : m_pOutStream(pOutStream) {}
	virtual ~C7ZipOutStreamWrap() {}

public:
	MY_UNKNOWN_IMP1(IOutStream)

	STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
	{
		return m_pOutStream->Seek(offset, seekOrigin, newPosition);
	}

#if MY_VER_MAJOR > 9 || (MY_VER_MAJOR == 9 && MY_VER_MINOR>=20)
	STDMETHOD(SetSize)(UInt64 newSize)
#else
	STDMETHOD(SetSize)(Int64 newSize)
#endif
	{
		return m_pOutStream->SetSize(newSize);
	}

	STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize)
	{
		return m_pOutStream->Write(data, size, processedSize);
	}

private:
	C7ZipOutStream * m_pOutStream;
};



class CArchiveOpenCallback:
    public IArchiveOpenCallback,
    public ICryptoGetTextPassword,
	public IArchiveOpenVolumeCallback,
    public CMyUnknownImp
{
public:
    MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

    STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes);
    STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes);

    STDMETHOD(CryptoGetTextPassword)(BSTR *password);

	// IArchiveOpenVolumeCallback
	STDMETHOD(GetProperty)(PROPID propID, PROPVARIANT *value);
	STDMETHOD(GetStream)(const wchar_t *name, IInStream **inStream);

    bool PasswordIsDefined;
    wstring Password;

    CArchiveOpenCallback() : PasswordIsDefined(false) {}
};

class CArchiveExtractCallback:
	public IArchiveExtractCallback,
	public ICryptoGetTextPassword,
	public CMyUnknownImp
{
public:
	MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

	// IProgress
	STDMETHOD(SetTotal)(UInt64 size);
	STDMETHOD(SetCompleted)(const UInt64 *completeValue);

	// IArchiveExtractCallback
	STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode);
	STDMETHOD(PrepareOperation)(Int32 askExtractMode);
	STDMETHOD(SetOperationResult)(Int32 resultEOperationResult);

	// ICryptoGetTextPassword
	STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);
	
private:
	C7ZipOutStreamWrap * _outFileStreamSpec;
	CMyComPtr<ISequentialOutStream> _outFileStream;

	C7ZipOutStream * m_pOutStream;
public:
	CArchiveExtractCallback(C7ZipOutStream * pOutStream) : 
		m_pOutStream(pOutStream)
	{
	}
};

class CInFileStreamWrap:
 public IInStream,
 public IStreamGetSize,
 public CMyUnknownImp
{
	private:
    C7ZipInStream * m_pInStream;

	public:
    CInFileStreamWrap(C7ZipInStream * pInStream);
    virtual ~CInFileStreamWrap() {}

	public:
    MY_UNKNOWN_IMP2(IInStream, IStreamGetSize)

        STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
    STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);

    STDMETHOD(GetSize)(UInt64 *size);

};

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
	switch(operationResult)
	{
	case NArchive::NExtract::NOperationResult::kOK:
		break;
	default:
		{
			switch(operationResult)
			{
			case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
				break;
			case NArchive::NExtract::NOperationResult::kCRCError:
				break;
			case NArchive::NExtract::NOperationResult::kDataError:
				break;
			default:
				break;
			}
		}
	}

	_outFileStream.Release();

	return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)
{
#ifdef _WIN32
	return StringToBstr(L"", password);
#else
	CMyComBSTR temp(L"");

	*password = temp.MyCopy();

	return S_OK;
#endif
}

#endif



#include "CPP/include_windows/windows.h"
#include "CPP/7zip/UI/Common/LoadCodecs.h"



extern "C" {
 int CreateObject(const GUID *clsid, const GUID *iid, void **outObject);
}

DEFINE_GUID(CLSID_CFormat7z,
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00);


struct Fuse7z_pimp {
	C7ZipInStream * stream;
	C7ZipArchive * archive;

	Fuse7z_pimp(const string & filename) {
		int err;

		if (CreateObject(&CLSID_CFormat7z, &IID_IInArchive, (void **)&archive) != S_OK)
		{
			stringstream ss;
			ss << "Could not load 7z handler";
			throw runtime_error(ss.str());
		}
		//stream = new TestInStream(archiveName);

		// open archive(stream, archive);
	}
};


#endif // defined(USE_P7Z)

#if 1


#include "lib7zip.h"

using namespace std;


class TestOutStream : public C7ZipOutStream
{
	public:
	vector<char> buffer;
	unsigned long long int position;
	TestOutStream() {

	}
	virtual ~TestOutStream() {
	}
	virtual int Write(const void *data, unsigned int size, unsigned int *processedSize) {
		logger << "Write " << data << " size=" << size << processedSize << Logger::endl;
		memcpy(buffer.data(), data, size);
		*processedSize = size;
		return 0;//throw runtime_error("Not implemented");
	}
	virtual int Seek(__int64 offset, unsigned int seekOrigin, unsigned __int64 *newPosition) {
		position = seekOrigin;
		//throw runtime_error("Not implemented");
		return 0;
	}
	virtual int SetSize(unsigned __int64 size) {
		buffer.resize(size);
		return 0;
	}
};

class TestInStream : public C7ZipInStream
{
private:
	FILE * m_pFile;
	std::string m_strFileName;
	std::wstring m_strFileExt;
	int m_nFileSize;
public:
	TestInStream(std::string const & fileName) : m_strFileName(fileName), m_strFileExt(L"7z") {

		logger << "Opening " << fileName << Logger::endl;
		m_pFile = fopen(fileName.c_str(), "rb");
		if (m_pFile) {
			fseek(m_pFile, 0, SEEK_END);
			m_nFileSize = ftell(m_pFile);
			fseek(m_pFile, 0, SEEK_SET);

			int pos = m_strFileName.find_last_of(".");

			if (pos != m_strFileName.npos) {
#ifdef _WIN32
				std::string tmp = m_strFileName.substr(pos + 1);
				int nLen = MultiByteToWideChar(CP_ACP, 0, tmp.c_str(), -1, NULL, NULL);
				LPWSTR lpszW = new WCHAR[nLen];
				MultiByteToWideChar(CP_ACP, 0,
									tmp.c_str(), -1, lpszW, nLen);
				m_strFileExt = lpszW;
				// free the string
				delete[] lpszW;
#else

				m_strFileExt.resize(fileName.length() - pos);
				for (int i(0); i < m_strFileExt.length(); i++) {
					m_strFileExt[i] = m_strFileName[pos+1+i];
				}
#endif
			}
			//logger << "ext:" << m_strFileExt << Logger::endl;
		}
		else {
			stringstream ss;
			ss << "Can't open " << fileName;
			throw runtime_error(ss.str());
		}
	}

	virtual ~TestInStream()
	{
		fclose(m_pFile);
	}

public:
	virtual std::wstring GetExt() const
	{
		return m_strFileExt;
	}

	virtual int Read(void *data, unsigned int size, unsigned int *processedSize)
	{
		if (!m_pFile)
			return 1;

		int count = fread(data, 1, size, m_pFile);
		if (count >= 0) {
			if (processedSize != NULL)
				*processedSize = count;

			return 0;
		}

		return 1;
	}

	virtual int Seek(long long int offset, unsigned int seekOrigin, unsigned long long int *newPosition)
	{
		if (!m_pFile)
			return 1;

		int result = fseek(m_pFile, (long)offset, seekOrigin);

		if (!result) {
			if (newPosition)
				*newPosition = ftell(m_pFile);

			return 0;
		}

		return result;
	}

	virtual int GetSize(unsigned long long int * size)
	{
		if (size)
			*size = m_nFileSize;
		return 0;
	}
};
const wchar_t * index_names[] = {
		L"kpidPackSize", //(Packed Size)
		L"kpidAttrib", //(Attributes)
		L"kpidCTime", //(Created)
		L"kpidATime", //(Accessed)
		L"kpidMTime", //(Modified)
		L"kpidSolid", //(Solid)
		L"kpidEncrypted", //(Encrypted)
		L"kpidUser", //(User)
		L"kpidGroup", //(Group)
		L"kpidComment", //(Comment)
		L"kpidPhySize", //(Physical Size)
		L"kpidHeadersSize", //(Headers Size)
		L"kpidChecksum", //(Checksum)
		L"kpidCharacts", //(Characteristics)
		L"kpidCreatorApp", //(Creator Application)
		L"kpidTotalSize", //(Total Size)
		L"kpidFreeSpace", //(Free Space)
		L"kpidClusterSize", //(Cluster Size)
		L"kpidVolumeName", //(Label)
		L"kpidPath", //(FullPath)
		L"kpidIsDir", //(IsDir)
};


class Fuse7z_lib7zip : public Fuse7z {
	C7ZipLibrary lib;
	C7ZipArchive * archive;
	TestInStream stream;
	public:

	Fuse7z_lib7zip(std::string const & filename, std::string const & cwd) : Fuse7z(filename, cwd), stream(filename) {
		logger << "Initialization of fuse-7z with archive " << filename << Logger::endl;

		if (!lib.Initialize()) {
			stringstream ss;
			ss << "7z library initialization failed";
			throw runtime_error(ss.str());
		}

		WStringArray exts;

		if (!lib.GetSupportedExts(exts)) {
			stringstream ss;
			ss << "Could not get list of 7z-supported file extensions";
			throw runtime_error(ss.str());
		}

		if (0) {
		size_t size = exts.size();
		for(size_t i = 0; i < size; i++) {
			std::wstring ext = exts[i];

			for(size_t j = 0; j < ext.size(); j++) {
				wprintf(L"%c", (char)(ext[j] &0xFF));
			}

			wprintf(L"\n");
		}
		}


		if (lib.OpenArchive(&stream, &archive)) {
			unsigned int numItems = 0;

			archive->GetItemCount(&numItems);

			logger << "Archive contains " << numItems << " entries" << Logger::endl;
			Node * node;
			for(unsigned int i = 0;i < numItems;i++) {
				C7ZipArchiveItem * pArchiveItem = NULL;
				if (archive->GetItemInfo(i, &pArchiveItem)) {
					std::wstring wpath(pArchiveItem->GetFullPath());
					std::string path;
					path.resize(wpath.length());
					std::copy(wpath.begin(), wpath.end(), path.begin());

					node = root_node->insert(const_cast<char*>(path.c_str()));
					node->id = i;

					node->is_dir = pArchiveItem->IsDir();

					{
						unsigned long long size;
						pArchiveItem->GetUInt64Property(lib7zip::kpidSize, size);
						node->stat.st_size = size;
					}

					{ // TODO fix the bad times
						unsigned long long time;
						pArchiveItem->GetFileTimeProperty(lib7zip::kpidATime, time);
						node->stat.st_atim.tv_sec = time / 1e8;
						node->stat.st_atim.tv_nsec = time % int(1e8);
						pArchiveItem->GetFileTimeProperty(lib7zip::kpidCTime, time);
						node->stat.st_ctim.tv_sec = time / 1e8;
						node->stat.st_ctim.tv_nsec = time % int(1e8);
						pArchiveItem->GetFileTimeProperty(lib7zip::kpidMTime, time);
						node->stat.st_mtim.tv_sec = time / 1e8;
						node->stat.st_mtim.tv_nsec = time % int(1e8);
					}
				}
				if ((i+1) % 10000 == 0) {
					logger << "Indexed " << (i+1) << "th file : " << node->fullname() << Logger::endl;
				}
			}
		}
		else {
			stringstream ss;
			ss << "open archive " << archive_fn  << " failed" << endl;
			throw runtime_error(ss.str());
		}

	}
	virtual ~Fuse7z_lib7zip() {
		delete archive;
		lib.Deinitialize();
	}
	
	virtual void open(char const * path, Node * node) {
		logger << "Opening file " << path << "(" << node->fullname() << ")" << Logger::endl;
	}
	virtual void close(char const * path, Node * node) {
		logger << "Closing file " << path << "(" << node->fullname() << ")" << Logger::endl;
	}
	virtual int read(char const * path, Node * node, char * buf, size_t size, off_t offset) {
		logger << "Reading file " << path << "(" << node->fullname() << ") for " << size << " at " << offset << ", arch_id=" << node->id << Logger::endl;
		int id = node->id;
		TestOutStream s;
		s.SetSize(node->stat.st_size);
		archive->Extract(id, &s);
		//s.Seek(0, 0, NULL);
		memcpy(buf, &s.buffer [offset], size);
		return size;
	}

};

extern "C" {
void * fuse7z_initlib(char const * archive, char const * cwd) {
	void * lib = new Fuse7z_lib7zip(archive, cwd);
	//logger.logger("pouet");
	return lib;
}
}




#endif

