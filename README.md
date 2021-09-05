# opendds.org

This is the source for [opendds.org](https://opendds.org). It uses [the Jekyll
static website generator](https://jekyllrb.com/).

## Building and Viewing Locally

Pushing to the `gh-pages` branch causes Github to update opendds.org. It can
also be built and viewed locally. To do this make sure Ruby is installed. Then
in this directory run these commands to install the dependencies:

```Shell
gem install bundler
bundle install
```

Then the website can be generated and a webserver started by running:

```Shell
bundle exec jekyll serve
```

It will print the URL of the locally hosted site.

### GitHub API Limits

Generating the site uses the GitHub API, which has limits for unauthorized
access. If you generate the site enough times in a certain period of time, then
it will prevent any further access and cause the generation to fail.

To get around this, create or reuse a [personal access token from a GitHub
account with at least the `public_repos`
permission](https://docs.github.com/en/github/authenticating-to-github/keeping-your-account-and-data-secure/creating-a-personal-access-token).
This will still have limitations, but they will be higher than unauthenticated
usage.

The token can be used by assigning it to the `JEKYLL_GITHUB_TOKEN` environment
variable. For example:

```Shell
JEKYLL_GITHUB_TOKEN=... bundle exec jekyll serve
```
