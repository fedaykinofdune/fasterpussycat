
#include "http_client.h"
#include "detect_404.h"
#include "post.h"
#include "find_uploaded_file.h"


static char *upload_dirs[] = {
  "/_uploads/",
  "/_upload/",
  "/uploads/",
  "/Uploads/",
  "/UPLOADS/",
  "/upload/",
  "/Upload/",
  "/UPLOAD/",
  "/images/",
  "/Images/",
  "/IMAGES/",
  "/img/",
  "/Img/",
  "/IMG/",
  "/pictures/", 
  "/Pictures/", 
  "/data/", 
  "/Data/", 
  "/files/",  
  "/Files/",  
  "/file/", 
  "/File/", 
  "/user/", 
  "/User/", 
  "/fileupload/",
  "/FileUpload/",
  "/file_upload/",  
  "/File_Upload/",  
  "/download/", 
  "/downloads/",  
  "/up/", 
  "/Up/", 
  "/UP/", 
  "/upld/", 
  "/down/", 
  "/templates/",
  "/temporary/",        
  "/temp/",
  "/Temp/",
  "/tmp/",
  0
};
  

void start_find_uploaded_file(struct target *t){
  find_uploaded_file(t, (unsigned char *) t->upload_file);
}

void find_uploaded_file(struct target *t, unsigned char *file){
 
  unsigned char *path=ck_alloc(strlen((char*) file)+100);
  int i;
  char *p=(char *) file;
  if(p[0]=='/') p++;
  struct http_request *req; 
  for(i=0;upload_dirs[i];i++){
      path[0]=0;
      strcat(path,upload_dirs[i]);
      strcat(path, p);
      req=new_request_with_method_and_path(t,"GET",path);
      req->callback=process_find_uploaded_file;
      async_request(req);
  }
  ck_free(path);
}

u8 process_find_uploaded_file(struct http_request *req, struct http_response *res){
  if(is_404(req->t->detect_404,req,res)!=DETECT_SUCCESS){
    return 0;
  }
  annotate(res,"found-uploaded-file", NULL);
  output_result(req,res);
  remove_host_from_queue_with_callback(req->t->full_host,process_find_uploaded_file);
  
  return 1;
}

