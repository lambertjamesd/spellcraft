#include <stdio.h>

#include <sys/inotify.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

struct inotify_event_header
{
  int wd;		/* Watch descriptor.  */
  uint32_t mask;	/* Watch mask.  */
  uint32_t cookie;	/* Cookie to synchronize two events.  */
  uint32_t len;		/* Length (including NULs) of name.  */
};

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        printf("usage %s <directory to watch>\n", argv[0]);
        return 1;
    }

    int notify_file = inotify_init();

    if (notify_file == -1) {
        fprintf(stderr, "Failed to call inotify_init errno = %s\n", strerror(errno));
        return 1;
    }

    int watch_result = inotify_add_watch(notify_file, "filesystem", IN_ATTRIB | IN_CREATE | IN_MOVED_TO | IN_MOVED_FROM | IN_MODIFY | IN_DELETE | IN_DELETE_SELF);

    if (watch_result == -1) {
        fprintf(stderr, "Failed to call inotify_add_watch errno = %s\n", strerror(errno));
        return 1;
    }

    bool isActive = true;

    while (isActive) {
        char event_buffer[sizeof(struct inotify_event) + 128 + 1];
        int read_size = read(notify_file, &event_buffer, sizeof(event_buffer));

        if (read_size == -1) {
            isActive = false;
            break;
        }
        
        char* curr = event_buffer;
        char* end = event_buffer + read_size;

        while (curr < end) {
            struct inotify_event event;
            memcpy(&event, event_buffer, sizeof(event));

            curr += sizeof(event);

            char name[event.len + 1];
            name[event.len] = '\0';

            memcpy(name, curr, event.len);
            curr += event.len;

            printf("mask = %08x\n", event.mask);
            printf("wd = %d\n", event.wd);
            printf("len = %d\n", event.len);
            printf("name = %s\n", name);
        }
    }

    close(notify_file);

    return 0;
}