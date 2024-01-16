if [[ $# -ne 3 ]]; then
    echo usage: $0 ref_name author_login body_file
    exit 1
fi
ref_name=$1
author_login=$2
body_file=$3

first_line=$(sed -n 1p ${body_file})
echo ${first_line}

## Get the ID of the previous comment, if present
comment_id=$( \
  gh pr view ${ref_name} \
    --json comments \
    --jq '[.comments[]
      | select(.author.login == "'"${author_login}"'")
      | select(.body|startswith("'"${first_line}"'"))
      | .url][0]' \
  | sed 's/^.*issuecomment-//')

echo ${comment_id}

# Update the comment if present, otherwise create one
if [[ -z "${comment_id}" ]]; then
    gh pr comment ${ref_name} --body-file ${body_file}
else
    gh api \
      --method PATCH \
      -H "Accept: application/vnd.github+json" \
      -H "X-GitHub-Api-Version: 2022-11-28" \
      /repos/{owner}/{repo}/issues/comments/${comment_id} \
      --field body=@${body_file}
fi
