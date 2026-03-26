# opendds.org

This is the source for [opendds.org](https://opendds.org). It uses [the Jekyll
static website generator](https://jekyllrb.com/).

## Building and Viewing Locally

Pushing to the `gh-pages` branch causes Github to update opendds.org. It can
also be built and viewed locally. For first time installs make sure Ruby is
installed and install bundler:

```
gem install bundler
# will need sudo on Unix-like systems
```

Run this command in this directory to install the dependencies:

```
bundle install
```

This may produce an error like:

```
Gem::Ext::BuildError: ERROR: Failed to build gem native extension.
```

On Debian/Ubuntu systems run `sudo apt install ruby-dev` and try again.

Then to generate the website and start a web server for it, run:

```
bundle exec jekyll serve
```

It will print the URL of the locally hosted site.

### Strict Build

This will command will catch errors with the Liquid templetess.

```
bundle exec jekyll build --config _config.yml,_config_strict.yml
```

## Running `htmlproofer.sh` Locally

[`htmlproofer.sh`](htmlproofer.sh) is a wrapper around
[html-proofer](https://github.com/gjtorikian/html-proofer).

It is run as part of GitHub Actions workflow on this branch, but can be run
locally by first building the site:

```
bundle exec jekyll build
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
