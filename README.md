# opendds.org

This is the source for [opendds.org](https://opendds.org). It uses [the Jekyll
static website generator](https://jekyllrb.com/).

## Building and Viewing Locally

Pushing to the `gh-pages` branch causes Github to update opendds.org. It can
also be built and viewed locally. To do this make sure Ruby is installed and
then run these commands in this directory to install the dependencies:

```
gem install bundler
bundle install
```

Then to generate the website and start a web server for it, run:

```
bundle exec jekyll serve
```

It will print the URL of the locally hosted site.

## Running `htmlproofer.sh` Locally

[`htmlproofer.sh'](htmlproofer.sh) is a wrapper around
[the html-proofer Ruby program](https://github.com/gjtorikian/html-proofer).

It is run as part of GitHub Actions workflow on this branch, but can be run
locally by first building the site:

```
bundle exec jekyll serve
```

A `\_site` directory will appear, and then the script can be run on it:

```
bash htmlproofer.sh
```

### GitHub API Limits

Generating the site uses the GitHub API, which [has limits for unauthorized
access](https://docs.github.com/en/rest/overview/resources-in-the-rest-api#rate-limiting).
If you generate the site enough times in a certain period of time, GitHub will
prevent any further access and this causes the generation to fail.

To get around this, create or use an existing [personal access token from a
GitHub account with at least the `public_repos`
permission](https://docs.github.com/en/github/authenticating-to-github/keeping-your-account-and-data-secure/creating-a-personal-access-token).
This will still have limitations, but they will be higher than unauthenticated
usage.

The token can be used by assigning it to the `JEKYLL_GITHUB_TOKEN` environment
variable. For example:

```
JEKYLL_GITHUB_TOKEN=... bundle exec jekyll serve
```

See [here](http://jekyll.github.io/github-metadata/authentication/) for more
information on GitHub authentication with Jekyll.
