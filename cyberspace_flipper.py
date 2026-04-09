import requests
import getpass
import textwrap
import re

BASE_URL = "https://api.cyberspace.online/v1"
EMAIL = "quackintosh@duck.com"

def strip_markdown(text):
    text = re.sub(r'!\[.*?\]\(.*?\)', '[image]', text)
    text = re.sub(r'\[([^\]]+)\]\(.*?\)', r'\1', text)
    text = re.sub(r'\+\+\[([^\]]+)\]\(.*?\)\+\+', r'\1', text)
    text = re.sub(r'\*\*(.*?)\*\*', r'\1', text)
    text = re.sub(r'\*(.*?)\*', r'\1', text)
    text = re.sub(r'```.*?```', '', text, flags=re.DOTALL)
    return text.strip()

def login(password):
    r = requests.post(f"{BASE_URL}/auth/login", json={
        "email": EMAIL,
        "password": password
    })
    data = r.json()
    if "data" in data:
        return data["data"]["idToken"]
    else:
        print("Login failed:", data)
        return None

def get_feed(token, cursor=None):
    params = {"limit": 10}
    if cursor:
        params["cursor"] = cursor
    r = requests.get(f"{BASE_URL}/posts", headers={
        "Authorization": f"Bearer {token}"
    }, params=params)
    data = r.json()
    return data.get("data", []), data.get("cursor")

def get_replies(token, post_id):
    r = requests.get(f"{BASE_URL}/posts/{post_id}/replies", headers={
        "Authorization": f"Bearer {token}"
    }, params={"limit": 20})
    return r.json().get("data", [])

def post_reply(token, post_id, content, parent_reply_id=""):
    r = requests.post(f"{BASE_URL}/replies", headers={
        "Authorization": f"Bearer {token}"
    }, json={
        "postId": post_id,
        "content": content,
        "parentReplyId": parent_reply_id
    })
    return r.status_code == 200

def display_post(post, idx, total):
    print("\n" + "="*40)
    replies = post.get('repliesCount', 0)
    reply_str = f"{replies} {'reply' if replies == 1 else 'replies'}"
    print(f"@{post['authorUsername']}  💬 {reply_str}")
    print("-"*40)
    content = strip_markdown(post.get('content', ''))
    print(textwrap.fill(content, width=40))
    topics = post.get('topics', [])
    if topics:
        print(f"\n#{' #'.join(topics)}")
    print(f"\n[{idx+1}/{total}] n=next p=prev o=open q=quit")

def display_replies(token, post):
    post_id = post['postId']
    replies = get_replies(token, post_id)
    if not replies:
        print("\nNo replies yet.")
        input("Press Enter to go back...")
        return

    idx = 0
    while True:
        r = replies[idx]
        print("\n" + "="*40)
        if r.get('parentReplyAuthor'):
            print(f"@{r['authorUsername']} → @{r['parentReplyAuthor']}")
        else:
            print(f"@{r['authorUsername']}")
        print("-"*40)
        content = strip_markdown(r.get('content', ''))
        print(textwrap.fill(content, width=40))
        saves = r.get('savesCount', 0)
        if saves:
            print(f"🔖 {saves}")
        print(f"\n[{idx+1}/{len(replies)}] n=next p=prev r=reply b=back")
        cmd = input("> ").strip().lower()
        if cmd == 'n' and idx < len(replies) - 1:
            idx += 1
        elif cmd == 'p' and idx > 0:
            idx -= 1
        elif cmd == 'r':
            content = input("Reply: ").strip()
            if content:
                ok = post_reply(token, post_id, content, r['replyId'])
                print("✓ Reply sent!" if ok else "✗ Failed to send.")
                input("Press Enter to continue...")
        elif cmd == 'b':
            break

def main():
    password = getpass.getpass("Cyberspace password: ")
    print("Logging in...")
    token = login(password)
    if not token:
        return

    print("Fetching feed...")
    posts, cursor = get_feed(token)
    if not posts:
        print("No posts found.")
        return

    idx = 0
    while True:
        display_post(posts[idx], idx, len(posts))
        cmd = input("> ").strip().lower()
        if cmd == 'n':
            if idx < len(posts) - 1:
                idx += 1
            else:
                print("Loading more...")
                new_posts, cursor = get_feed(token, cursor)
                if new_posts:
                    posts.extend(new_posts)
                    idx += 1
                else:
                    print("No more posts.")
        elif cmd == 'p' and idx > 0:
            idx -= 1
        elif cmd == 'o':
            display_replies(token, posts[idx])
        elif cmd == 'q':
            break

if __name__ == "__main__":
    main()