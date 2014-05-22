#ifndef DB_H_
#define DB_H_


typedef struct Usr_Record{
       int32_t UserId;
	char UserName[128];
	char Password[128];
	time_t LastLoginTime;
	time_t LastLogoutTime;
}UsrRecord;

typedef struct Object_Record{
	int64_t ObjectId;
	int32_t UserId;
	char ObjectName[128];
	char fileset[FILE_NAME_LEN];
	int32_t FullCount;
	int32_t IncrementCount;
	time_t LastBackupTime;
}ObjectRecord;

typedef struct Job_Record{
       int64_t JobId;
	int32_t UserId;
	int64_t ObjectId;
	char JobType;
	time_t StartTime;
	time_t FinishTime;
	int32_t FileCount;
	int64_t DataSize;
}JobRecord;

typedef struct File_Record{
	int64_t FileId;
	int64_t JobId;
	int32_t VersionCount;
	char FilePath[FILE_NAME_LEN];
	Fingerprint HashCode;
	int64_t Size;
	time_t ModifiedTime;
}FileRecord;

typedef struct Version_File_Record{
	int64_t FileId;
	int64_t JobId;
	char FilePath[FILE_NAME_LEN];
}VersionFileRecord;

bool find_username(DedupDb *mdb, char *username) ;

bool get_pass_by_username(DedupDb *mdb, char *username, char *password);

bool insert_record_into_user(DedupDb *mdb, UsrRecord *user);

bool insert_record_into_object(DedupDb *mdb, ObjectRecord *object);

bool insert_record_into_job(DedupDb *mdb, JobRecord* job);

bool insert_record_into_file(DedupDb *mdb, FileRecord *file);

bool insert_record_into_versionfile(DedupDb *mdb, VersionFileRecord *versionfile);

//bool insert_record_into_segment(DedupDb *mdb);


#endif /*DB_H_*/
