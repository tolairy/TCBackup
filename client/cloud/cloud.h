#ifndef CLOUD_H_
#define CLOUD_H_

void cloud_upload_init();
/*you can call this two functions,
 * if you want to upload or download segments,
 * this two functions are both non-blocking*/
bool cloud_upload(char *name);

void cloud_upload_destory();

void cloud_download_init();

bool cloud_download(char *name);

void cloud_download_destory();

#endif
