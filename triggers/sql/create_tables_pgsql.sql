-- Postgres specific configuration script

Create Table %PREFIX%SessionLog (Id Serial Primary Key Not Null,
	Command varchar(32),
	Date timestamp,
	Hostname varchar(256),
	Username varchar(256),
	SessionId varchar(32),
	VirtRepos varchar(256),
	PhysRepos varchar(256),
	Client varchar(64));

Create Table %PREFIX%CommitLog (Id Serial Primary Key Not Null,
	SessionId Integer,
	Directory varchar(256),
	Message text,
	Type char(1),
	Filename varchar(256),
	Tag varchar(64),
	BugId varchar(64),
	OldRev varchar(64),
	NewRev varchar(64),
	Added Integer,
	Removed Integer,
	Diff text);

Create Index %PREFIX%Commit_SessionId On %PREFIX%CommitLog(SessionId);

Create Table %PREFIX%HistoryLog (Id Serial Primary Key Not Null,
	SessionId Integer,
	Type char(1),
	WorkDir varchar(256),
	Revs varchar(64),
	Name varchar(256),
	BugId varchar(64),
	Message text);

Create Index %PREFIX%History_SessionId on %PREFIX%HistoryLog(SessionId);

Create Table %PREFIX%TagLog (Id Serial Primary Key Not Null,
	SessionId Integer,
	Directory varchar(256),
	Filename varchar(256),
	Tag varchar(64),
	Revision varchar(64),
	Message text,
	Action varchar(32),
	Type char(1));
	
Create Index %PREFIX%Tag_SessionId on %PREFIX%TagLog(SessionId);
