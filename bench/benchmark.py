from locust import HttpLocust, TaskSet, task
from locust.user.users import HttpUser

class WebsiteTasks(TaskSet):
    @task
    def aaa(self):
        self.client.get("/index.png")
    
class WebsiteUser(HttpUser):
    tasks = [WebsiteTasks]
    host = "http://172.20.90.16:10000"
    min_wait = 1000
    max_wait = 5000